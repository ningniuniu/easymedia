/*
 * Copyright (C) 2017 Hertz Wang wangh@rock-chips.com
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
 * Commercial non-GPL licensing of this software is possible.
 * For more info contact Rockchip Electronics Co., Ltd.
 */

#include "buffer.h"

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "key_string.h"
#include "utils.h"

namespace rkmedia {

MediaBuffer::MemType StringToMemType(const char *s) {
  if (s) {
#ifdef LIBION
    if (!strcmp(s, KEY_MEM_ION))
      return MediaBuffer::MemType::MEM_ION;
#endif
#ifdef LIBDRM
    if (!strcmp(s, KEY_MEM_DRM))
      return MediaBuffer::MemType::MEM_DRM;
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

static std::shared_ptr<rkmedia::MediaBuffer> alloc_common_memory(size_t size) {
  void *buffer = malloc(size);
  if (!buffer)
    return nullptr;
  return std::make_shared<rkmedia::MediaBuffer>(buffer, size, -1, buffer,
                                                free_common_memory);
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

static std::shared_ptr<rkmedia::MediaBuffer> alloc_ion_memory(size_t size) {
  int client = ion_open();
  if (client < 0) {
    LOG("ion_open() failed: %m\n");
    return nullptr;
  }
  ion_user_handle_t handle;
  int ret = ion_alloc(client, size, 0, ION_HEAP_TYPE_DMA_MASK, 0, &handle);
  if (ret) {
    LOG("ion_alloc() failed: %m\n");
    ion_close(client);
    return nullptr;
  }
  int fd;
  ret = ion_share(client, handle, &fd);
  if (ret < 0) {
    LOG("ion_share() failed: %m\n");
    ion_free(client, handle);
    ion_close(client);
    return nullptr;
  }
  void *ptr =
      mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, fd, 0);
  if (!ptr) {
    LOG("ion mmap() failed: %m\n");
    ion_free(client, handle);
    ion_close(client);
    close(fd);
    return nullptr;
  }
  IonBuffer *buffer = new IonBuffer(client, handle, fd, ptr, size);
  if (!buffer) {
    ion_free(client, handle);
    ion_close(client);
    close(fd);
    munmap(ptr, size);
    return nullptr;
  }

  return std::make_shared<rkmedia::MediaBuffer>(ptr, size, fd, buffer,
                                                free_ion_memory);
}
#endif

std::shared_ptr<rkmedia::MediaBuffer> MediaBuffer::Alloc(size_t size,
                                                         MemType type) {
  switch (type) {
  case MemType::MEM_COMMON:
    return alloc_common_memory(size);
#ifdef LIBION
  case MemType::MEM_ION:
    return alloc_ion_memory(size);
#endif
#ifdef LIBDRM
  case MemType::MEM_DRM:
    return alloc_drm_memory(size);
#endif
  default:
    LOG("unknown memtype\n");
    return nullptr;
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
  new_buffer->CopyAttribute(src);
  return new_buffer;
}

void MediaBuffer::CopyAttribute(MediaBuffer &src_attr) {
  valid_size = src_attr.GetValidSize();
  type = src_attr.GetType();
  user_flag = src_attr.GetUserFlag();
  timestamp = src_attr.GetTimeStamp();
  eof = src_attr.IsEOF();
}

} // namespace rkmedia
