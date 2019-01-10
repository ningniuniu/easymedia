/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: hertz.wang hertz.wong@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_BUFFER_H_
#define RKMEDIA_BUFFER_H_

#include <stdint.h>
#include <sys/time.h>

#ifdef LIBION
#include <ion/ion.h>
#endif
#ifdef LIBDRM
#error(__FILE__:__LINE__): drm TODO
#endif

#include "image.h"
#include <stdarg.h>
#include <stdio.h>

typedef int (*DeleteFun)(void *arg);

namespace rkmedia {

void LOG(const char *format, ...) {
  char line[1024];
  va_list vl;
  va_start(vl, format);
  vsnprintf(line, sizeof(line), format, vl);
  va_end(vl);
}

#define LOG_NO_MEMORY()                                                        \
  fprintf(stderr, "No memory %s: %d\n", __FUNCTION__, __LINE__)

// For hardware, alway need specail buffer.
// HwMediaBuffer is designed for wrapping existing buffer.
class HwMediaBuffer {
public:
  // Set userdata and delete function if you want free resource when destrut.
  HwMediaBuffer(int buffer_fd, void *buffer_ptr, size_t buffer_size,
                void *user_data = nullptr, DeleteFun df = nullptr)
      : fd(buffer_fd), ptr(buffer_ptr), size(buffer_size), valid_size(0),
        user_flag(0), timestamp(0), userdata(user_data), delete_userdata(df) {}
  virtual ~HwMediaBuffer() {
    if (userdata && delete_userdata)
      (*delete_userdata)(userdata);
  }
  int GetFD() const { return fd; }
  void *GetPtr() const { return ptr; }
  size_t GetSize() const { return size; }
  size_t GetValidSize() const { return valid_size; }
  void SetValidSize(size_t s) { valid_size = s; }
  uint32_t GetUserFlag() const { return user_flag; }
  void SetUserFlag(uint32_t flag) { user_flag = flag; }
  int64_t GetTimeStamp() const { return timestamp; }
  void SetTimeStamp(int64_t ts) { timestamp = ts; }

  bool IsValid() { return valid_size > 0; }

private:
  int fd;    // buffer fd
  void *ptr; // buffer mmap virtual address
  size_t size;
  size_t valid_size; // valid data size, less than above size
  uint32_t user_flag;
  int64_t timestamp;

  void *userdata; // compatible with C
  DeleteFun delete_userdata;
};

class HwImageBuffer : public HwMediaBuffer {
public:
  HwImageBuffer() : HwMediaBuffer(-1, nullptr, 0) {}
  HwImageBuffer(HwMediaBuffer &buffer, ImageInfo &info)
      : HwMediaBuffer(buffer), image_info(info) {}
  virtual ~HwImageBuffer() = default;

  int GetWidth() const { return image_info.width; }
  int GetHeight() const { return image_info.height; }
  int GetVirWidth() const { return image_info.vir_width; }
  int GetVirHeight() const { return image_info.vir_height; }
  // PixelFormat GetPixelFormat const { return image_info.pix_fmt; }

private:
  ImageInfo image_info;
};

} // namespace rkmedia

#endif // RKMEDIA_BUFFER_H_
