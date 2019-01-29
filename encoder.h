/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_ENCODER_H_
#define RKMEDIA_ENCODER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

const uint8_t *find_h264_startcode(const uint8_t *p, const uint8_t *end);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include "codec.h"
#include "reflector.h"

#include <algorithm>

namespace rkmedia {

DECLARE_FACTORY(Encoder)

// usage: REFLECTOR(Encoder)::Create<T>(codecname, param)
// T must be the final class type exposed to user
DECLARE_REFLECTOR(Encoder)

#define DEFINE_ENCODER_FACTORY(REAL_PRODUCT, FINAL_EXPOSE_PRODUCT)             \
  DEFINE_CHILD_FACTORY(REAL_PRODUCT, REAL_PRODUCT::GetCodecName(),             \
                       FINAL_EXPOSE_PRODUCT, Encoder)

#define DEFINE_VIDEO_ENCODER_FACTORY(REAL_PRODUCT)                             \
  DEFINE_ENCODER_FACTORY(REAL_PRODUCT, VideoEncoder)

class Encoder : public Codec {
public:
  virtual ~Encoder() = default;
  virtual bool InitConfig(const MediaConfig &cfg) = 0;
};

// self define by user
class ParameterBuffer {
public:
  ParameterBuffer(size_t st) : size(st), ptr(nullptr), user_data(nullptr) {
    if (sizeof(int) != st)
      ptr = malloc(st);
  }
  ~ParameterBuffer() {
    if (ptr)
      free(ptr);
  }
  size_t GetSize() { return size; }
  int GetValue() { return value; }
  void SetValue(int v) { value = v; }
  void *GetBufferPtr() { return ptr; }
  void *GetUserData() { return user_data; }
  void SetUserData(void *data) { user_data = data; }

private:
  size_t size;
  int value;
  void *ptr;
  void *user_data;
};

class VideoEncoder : public Encoder {
public:
  // changes
  static const uint32_t kQPChange = (1 << 0);
  static const uint32_t kFrameRateChange = (1 << 1);
  static const uint32_t kBitRateChange = (1 << 2);
  static const uint32_t kForceIdrFrame = (1 << 3);
  static const uint32_t kOSDDataChange = (1 << 4);

  virtual ~VideoEncoder() = default;
  void RequestChange(uint32_t change, std::shared_ptr<ParameterBuffer> value);
  virtual bool InitConfig(const MediaConfig &cfg) override;

protected:
  bool HasChangeReq() { return !change_list.empty(); }
  std::pair<uint32_t, std::shared_ptr<ParameterBuffer>> PeekChange();

private:
  std::mutex change_mtx;
  std::list<std::pair<uint32_t, std::shared_ptr<ParameterBuffer>>> change_list;

  DECLARE_PART_FINAL_EXPOSE_PRODUCT(Encoder)
};

} // namespace rkmedia

#endif

#endif // RKMEDIA_ENCODER_H_
