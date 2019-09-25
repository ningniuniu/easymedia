/*
 * Copyright (C) 2017 Hertz Wang 1989wanghang@163.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses
 *
 * Any non-GPL usage of this software or parts of this software is strictly
 * forbidden.
 *
 */

#include "buffer.h"

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "key_string.h"
#include "utils.h"

namespace easymedia {

MediaBuffer::MemType StringToMemType(const char *s) {
  if (s) {
#ifdef LIBION
    if (!strcmp(s, KEY_MEM_ION) || !strcmp(s, KEY_MEM_HARDWARE))
      return MediaBuffer::MemType::MEM_HARD_WARE;
#endif
#ifdef LIBDRM
    if (!strcmp(s, KEY_MEM_DRM) || !strcmp(s, KEY_MEM_HARDWARE))
      return MediaBuffer::MemType::MEM_HARD_WARE;
#endif
    LOG("warning: %s is not supported or not integrated, fallback to common\n",
        s);
  }
  return MediaBuffer::MemType::MEM_COMMON;
}

static int free_common_memory(void *buffer) {
  if (buffer)
    free(buffer);
  return 0;
}

static MediaBuffer alloc_common_memory(size_t size) {
  void *buffer = malloc(size);
  if (!buffer)
    return MediaBuffer();
  return MediaBuffer(buffer, size, -1, buffer, free_common_memory);
}

#ifdef LIBION
#include <ion/ion.h>
class IonBuffer {
public:
  IonBuffer(int param_client, ion_user_handle_t param_handle, int param_fd,
            void *param_map_ptr, size_t param_len)
      : client(param_client), handle(param_handle), fd(param_fd),
        map_ptr(param_map_ptr), len(param_len) {}
  ~IonBuffer();

private:
  int client;
  ion_user_handle_t handle;
  int fd;
  void *map_ptr;
  size_t len;
};

IonBuffer::~IonBuffer() {
  if (map_ptr)
    munmap(map_ptr, len);
  if (fd >= 0)
    close(fd);
  if (client < 0)
    return;
  if (handle) {
    int ret = ion_free(client, handle);
    if (ret)
      LOG("ion_free() failed <handle: %d>: %m!\n", handle);
  }
  ion_close(client);
}

static int free_ion_memory(void *buffer) {
  assert(buffer);
  IonBuffer *ion_buffer = static_cast<IonBuffer *>(buffer);
  delete ion_buffer;
  return 0;
}

static MediaBuffer alloc_ion_memory(size_t size) {
  ion_user_handle_t handle;
  int ret;
  int fd;
  void *ptr;
  IonBuffer *buffer;
  int client = ion_open();
  if (client < 0) {
    LOG("ion_open() failed: %m\n");
    goto err;
  }
  ret = ion_alloc(client, size, 0, ION_HEAP_TYPE_DMA_MASK, 0, &handle);
  if (ret) {
    LOG("ion_alloc() failed: %m\n");
    ion_close(client);
    goto err;
  }
  ret = ion_share(client, handle, &fd);
  if (ret < 0) {
    LOG("ion_share() failed: %m\n");
    ion_free(client, handle);
    ion_close(client);
    goto err;
  }
  ptr =
      mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, fd, 0);
  if (!ptr) {
    LOG("ion mmap() failed: %m\n");
    ion_free(client, handle);
    ion_close(client);
    close(fd);
    goto err;
  }
  buffer = new IonBuffer(client, handle, fd, ptr, size);
  if (!buffer) {
    ion_free(client, handle);
    ion_close(client);
    close(fd);
    munmap(ptr, size);
    goto err;
  }

  return MediaBuffer(ptr, size, fd, buffer, free_ion_memory);
err:
  return MediaBuffer();
}
#endif

#ifdef LIBDRM

#include <drm_fourcc.h>
#include <xf86drm.h>
/**
 * Copy from libdrm_macros.h while is not exposed by libdrm,
 * be replaced by #include "libdrm_macros.h" someday.
 */
/**
 * Static (compile-time) assertion.
 * Basically, use COND to dimension an array.  If COND is false/zero the
 * array size will be -1 and we'll get a compilation error.
 */
#define STATIC_ASSERT(COND)                                                    \
  do {                                                                         \
    (void)sizeof(char[1 - 2 * !(COND)]);                                       \
  } while (0)

#if defined(ANDROID) && !defined(__LP64__)
#include <errno.h> /* for EINVAL */

extern void *__mmap2(void *, size_t, int, int, int, size_t);

static inline void *drm_mmap(void *addr, size_t length, int prot, int flags,
                             int fd, loff_t offset) {
  /* offset must be aligned to 4096 (not necessarily the page size) */
  if (offset & 4095) {
    errno = EINVAL;
    return MAP_FAILED;
  }

  return __mmap2(addr, length, prot, flags, fd, (size_t)(offset >> 12));
}

#define drm_munmap(addr, length) munmap(addr, length)

#else

/* assume large file support exists */
#define drm_mmap(addr, length, prot, flags, fd, offset)                        \
  mmap(addr, length, prot, flags, fd, offset)

static inline int drm_munmap(void *addr, size_t length) {
  /* Copied from configure code generated by AC_SYS_LARGEFILE */
#define LARGE_OFF_T ((((off_t)1 << 31) << 31) - 1 + (((off_t)1 << 31) << 31))
  STATIC_ASSERT(LARGE_OFF_T % 2147483629 == 721 &&
                LARGE_OFF_T % 2147483647 == 1);
#undef LARGE_OFF_T

  return munmap(addr, length);
}
#endif

// default
static int card_index = 0;
static int drm_device_open(const char *device = nullptr) {
  drmVersionPtr version;
  char drm_dev[] = "/dev/dri/card0000";
  uint64_t has_dumb;

  if (!device) {
    snprintf(drm_dev, sizeof(drm_dev), DRM_DEV_NAME, DRM_DIR_NAME, card_index);
    device = drm_dev;
  }
  int fd = open(device, O_RDWR);
  if (fd < 0)
    return fd;
  version = drmGetVersion(fd);
  if (!version) {
    LOG("Failed to get version information "
        "from %s: probably not a DRM device?\n",
        device);
    close(fd);
    return -1;
  }
  LOG("Opened DRM device %s: driver %s "
      "version %d.%d.%d.\n",
      device, version->name, version->version_major, version->version_minor,
      version->version_patchlevel);
  drmFreeVersion(version);
  if (drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 || !has_dumb) {
    LOG("drm device '%s' "
        "does not support dumb buffers\n",
        device);
    close(fd);
    return -1;
  }
  return fd;
}

class DrmDevice {
public:
  DrmDevice() { fd = drm_device_open(); }
  ~DrmDevice() {
    if (fd >= 0)
      close(fd);
  }
  bool Valid() { return fd >= 0; }

  int fd;
};

class DrmBuffer {
public:
  DrmBuffer(std::shared_ptr<DrmDevice> dev, size_t s, __u32 flags = 0)
      : device(dev), handle(0), len(UPALIGNTO(s, PAGE_SIZE)), fd(-1),
        map_ptr(nullptr) {
    struct drm_mode_create_dumb dmcb;
    memset(&dmcb, 0, sizeof(struct drm_mode_create_dumb));
    dmcb.bpp = 8;
    dmcb.width = len;
    dmcb.height = 1;
    dmcb.flags = flags;
    int ret = drmIoctl(dev->fd, DRM_IOCTL_MODE_CREATE_DUMB, &dmcb);
    if (ret < 0) {
      LOG("Failed to create dumb<w,h,bpp: %d,%d,%d>: %m\n", dmcb.width,
          dmcb.height, dmcb.bpp);
      return;
    }
    assert(dmcb.handle > 0);
    assert(dmcb.size >= dmcb.width * dmcb.height * dmcb.bpp / 8);
    handle = dmcb.handle;
    len = dmcb.size;
    ret = drmPrimeHandleToFD(dev->fd, dmcb.handle, DRM_CLOEXEC, &fd);
    if (ret) {
      LOG("Failed to convert drm handle to fd: %m\n");
      return;
    }
    assert(fd >= 0);
  }
  ~DrmBuffer() {
    if (map_ptr)
      drm_munmap(map_ptr, len);
    int ret;
    if (handle > 0) {
      struct drm_mode_destroy_dumb data = {
          .handle = handle,
      };
      ret = drmIoctl(device->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &data);
      if (ret)
        LOG("Failed to free drm handle <%d>: %m\n", handle);
    }
    if (fd >= 0) {
      ret = close(fd);
      if (ret)
        LOG("Failed to close drm buffer fd <%d>: %m\n", fd);
    }
  }
  bool MapToVirtual() {
    struct drm_mode_map_dumb dmmd;
    memset(&dmmd, 0, sizeof(dmmd));
    dmmd.handle = handle;
    int ret = drmIoctl(device->fd, DRM_IOCTL_MODE_MAP_DUMB, &dmmd);
    if (ret) {
      LOG("Failed to map dumb: %m\n");
      return false;
    }
    // default read and write
    void *ptr = drm_mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED,
                         device->fd, dmmd.offset);
    if (ptr == MAP_FAILED) {
      LOG("Failed to drm_mmap: %m\n");
      return false;
    }
    assert(ptr);
    map_ptr = ptr;
    return true;
  }
  bool Valid() { return fd >= 0; }

  std::shared_ptr<DrmDevice> device;
  __u32 handle;
  size_t len;
  int fd;
  void *map_ptr;
};

static int free_drm_memory(void *buffer) {
  assert(buffer);
  delete static_cast<DrmBuffer *>(buffer);
  return 0;
}

static MediaBuffer alloc_drm_memory(size_t size, bool map = true) {
  static auto drm_dev = std::make_shared<DrmDevice>();
  DrmBuffer *db = nullptr;
  do {
    if (!drm_dev || !drm_dev->Valid())
      break;
    db = new DrmBuffer(drm_dev, size);
    if (!db || !db->Valid())
      break;
    if (map && !db->MapToVirtual())
      break;
    return MediaBuffer(db->map_ptr, db->len, db->fd, db, free_drm_memory);
  } while (false);
  if (db)
    delete db;
  return MediaBuffer();
}
#endif

std::shared_ptr<MediaBuffer> MediaBuffer::Alloc(size_t size, MemType type) {
  MediaBuffer &&mb = Alloc2(size, type);
  if (mb.GetSize() == 0)
    return nullptr;
  return std::make_shared<MediaBuffer>(mb);
}

MediaBuffer MediaBuffer::Alloc2(size_t size, MemType type) {
  switch (type) {
  case MemType::MEM_COMMON:
    return alloc_common_memory(size);
#ifdef LIBION
  case MemType::MEM_HARD_WARE:
    return alloc_ion_memory(size);
#endif
#ifdef LIBDRM
  case MemType::MEM_HARD_WARE:
    return alloc_drm_memory(size);
#endif
  default:
    LOG("unknown memtype\n");
    return MediaBuffer();
  }
}

std::shared_ptr<MediaBuffer> MediaBuffer::Clone(MediaBuffer &src,
                                                MemType dst_type) {
  size_t size = src.GetValidSize();
  if (!size)
    return nullptr;
  auto new_buffer = Alloc(size, dst_type);
  if (!new_buffer) {
    LOG_NO_MEMORY();
    return nullptr;
  }
  if (src.IsHwBuffer() && new_buffer->IsHwBuffer())
    LOG_TODO(); // TODO: fd -> fd by RGA
  memcpy(new_buffer->GetPtr(), src.GetPtr(), size);
  new_buffer->SetValidSize(size);
  new_buffer->CopyAttribute(src);
  return new_buffer;
}

void MediaBuffer::CopyAttribute(MediaBuffer &src_attr) {
  type = src_attr.GetType();
  user_flag = src_attr.GetUserFlag();
  ustimestamp = src_attr.GetUSTimeStamp();
  eof = src_attr.IsEOF();
}

} // namespace easymedia
