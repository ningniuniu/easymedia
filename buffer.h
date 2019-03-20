/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang hertz.wong@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_BUFFER_H_
#define RKMEDIA_BUFFER_H_

#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#include <memory>

#include "image.h"
#include "media_type.h"
#include "sound.h"

typedef int (*DeleteFun)(void *arg);

namespace rkmedia {

// wrapping existing buffer
class MediaBuffer {
public:
  // video flags
  static const uint32_t kExtraIntra = (1 << 0); // special, such as sps pps
  static const uint32_t kIntra = (1 << 1);
  static const uint32_t kPredicted = (1 << 2);
  static const uint32_t kBiPredictive = (1 << 3);
  static const uint32_t kBiDirectional = (1 << 4);

  MediaBuffer()
      : ptr(nullptr), size(0), fd(-1), valid_size(0), type(Type::None),
        user_flag(0), timestamp(0), eof(false) {}
  // Set userdata and delete function if you want free resource when destrut.
  MediaBuffer(void *buffer_ptr, size_t buffer_size, int buffer_fd = -1,
              void *user_data = nullptr, DeleteFun df = nullptr)
      : ptr(buffer_ptr), size(buffer_size), fd(buffer_fd), valid_size(0),
        type(Type::None), user_flag(0), timestamp(0), eof(false) {
    SetUserData(user_data, df);
  }
  virtual ~MediaBuffer() = default;
  virtual PixelFormat GetPixelFormat() const { return PIX_FMT_NONE; }
  virtual SampleFormat GetSampleFormat() const { return SAMPLE_FMT_NONE; }
  int GetFD() const { return fd; }
  void SetFD(int new_fd) { fd = new_fd; }
  void *GetPtr() const { return ptr; }
  void SetPtr(void *addr) { ptr = addr; }
  size_t GetSize() const { return size; }
  void SetSize(size_t s) { size = s; }
  size_t GetValidSize() const { return valid_size; }
  void SetValidSize(size_t s) { valid_size = s; }
  Type GetType() const { return type; }
  void SetType(Type t) { type = t; }
  uint32_t GetUserFlag() const { return user_flag; }
  void SetUserFlag(uint32_t flag) { user_flag = flag; }
  int64_t GetTimeStamp() const { return timestamp; }
  struct timeval GetTimeVal() const {
    // timestamp always milliseconds ?
    struct timeval ret;
    ret.tv_sec = timestamp / 1000;
    ret.tv_usec = (timestamp % 1000) * 1000;
    return ret;
  }
  void SetTimeStamp(int64_t ts) { timestamp = ts; }
  bool IsEOF() const { return eof; }
  void SetEOF(bool val) { eof = val; }

  void SetUserData(void *user_data, DeleteFun df) {
    if (user_data) {
      if (df)
        userdata.reset(user_data, df);
      else
        userdata.reset(user_data, [](void *) {}); // do nothing when delete
    }
  }

  std::shared_ptr<void> GetUserData() { return userdata; }

  bool IsValid() { return valid_size > 0; }
  bool IsHwBuffer() { return fd >= 0; }

  enum class MemType {
    MEM_COMMON,
#ifdef LIBION
    MEM_ION,
#endif
#ifdef LIBDRM
#error(__FILE__:__LINE__): drm TODO
    MEM_DRM,
#endif
  };
  static std::shared_ptr<MediaBuffer> Alloc(size_t size,
                                            MemType type = MemType::MEM_COMMON);
  static std::shared_ptr<MediaBuffer>
  Clone(MediaBuffer &src, MemType dst_type = MemType::MEM_COMMON);

private:
  // copy attributs except buffer
  void CopyAttribute(MediaBuffer &src_attr);

  void *ptr; // buffer virtual address
  size_t size;
  int fd;            // buffer fd
  size_t valid_size; // valid data size, less than above size
  Type type;
  uint32_t user_flag;
  int64_t timestamp;
  bool eof;

  std::shared_ptr<void> userdata;
};

MediaBuffer::MemType StringToMemType(const char *s);

// Audio sample buffer
class SampleBuffer : public MediaBuffer {
public:
  SampleBuffer(const MediaBuffer &buffer, const SampleInfo &info)
      : MediaBuffer(buffer), sample_info(info) {
    SetType(Type::Audio);
  }
  virtual ~SampleBuffer() = default;
  virtual SampleFormat GetSampleFormat() const override {
    return sample_info.fmt;
  }

  SampleInfo &GetSampleInfo() { return sample_info; }
  size_t GetFrameSize() const { return ::GetFrameSize(sample_info); }
  void SetFrames(int num) {
    sample_info.frames = num;
    SetValidSize(num * GetFrameSize());
  }
  int GetFrames() const { return sample_info.frames; }

private:
  SampleInfo sample_info;
};

// Image buffer
class ImageBuffer : public MediaBuffer {
public:
  ImageBuffer() {
    SetType(Type::Image);
    memset(&image_info, 0, sizeof(image_info));
    image_info.pix_fmt = PIX_FMT_NONE;
  }
  ImageBuffer(const MediaBuffer &buffer, const ImageInfo &info)
      : MediaBuffer(buffer), image_info(info) {
    SetType(Type::Image);
  }
  virtual ~ImageBuffer() = default;
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
