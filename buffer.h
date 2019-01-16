/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang hertz.wong@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_BUFFER_H_
#define RKMEDIA_BUFFER_H_

#include <stdint.h>
#include <sys/time.h>

#include <memory>

#include "image.h"

typedef int (*DeleteFun)(void *arg);

namespace rkmedia {

// For hardware, alway need specail buffer.
// HwMediaBuffer is designed for wrapping existing buffer.
class HwMediaBuffer {
public:
  // Set userdata and delete function if you want free resource when destrut.
  HwMediaBuffer(int buffer_fd, void *buffer_ptr, size_t buffer_size,
                void *user_data = nullptr, DeleteFun df = nullptr)
      : fd(buffer_fd), ptr(buffer_ptr), size(buffer_size), valid_size(0),
        user_flag(0), timestamp(0), eof(false), userdata(nullptr) {
    if (user_data) {
      userdata.reset(user_data, df);
    }
  }
  virtual ~HwMediaBuffer() = default;
  int GetFD() const { return fd; }
  void SetFD(int new_fd) { fd = new_fd; }
  void *GetPtr() const { return ptr; }
  void SetPtr(void *addr) { ptr = addr; }
  size_t GetSize() const { return size; }
  void SetSize(size_t s) { size = s; }
  size_t GetValidSize() const { return valid_size; }
  void SetValidSize(size_t s) { valid_size = s; }
  uint32_t GetUserFlag() const { return user_flag; }
  void SetUserFlag(uint32_t flag) { user_flag = flag; }
  int64_t GetTimeStamp() const { return timestamp; }
  void SetTimeStamp(int64_t ts) { timestamp = ts; }
  bool IsEOF() const { return eof; }
  void SetEOF(bool val) { eof = val; }

  void SetUserData(void *user_data, DeleteFun df) {
    userdata.reset(user_data, df);
  }

  bool IsValid() { return valid_size > 0; }

  virtual PixelFormat GetPixelFormat() const { return PIX_FMT_NONE; }

private:
  int fd;    // buffer fd
  void *ptr; // buffer mmap virtual address
  size_t size;
  size_t valid_size; // valid data size, less than above size
  uint32_t user_flag;
  int64_t timestamp;
  bool eof;

  std::shared_ptr<void> userdata;
};

class HwImageBuffer : public HwMediaBuffer {
public:
  HwImageBuffer() : HwMediaBuffer(-1, nullptr, 0) {}
  HwImageBuffer(const HwMediaBuffer &buffer, const ImageInfo &info)
      : HwMediaBuffer(buffer), image_info(info) {}
  virtual ~HwImageBuffer() = default;

  virtual PixelFormat GetPixelFormat() const override {
    return image_info.pix_fmt;
  }
  int GetWidth() const { return image_info.width; }
  int GetHeight() const { return image_info.height; }
  int GetVirWidth() const { return image_info.vir_width; }
  int GetVirHeight() const { return image_info.vir_height; }

private:
  ImageInfo image_info;
};

} // namespace rkmedia

#endif // RKMEDIA_BUFFER_H_
