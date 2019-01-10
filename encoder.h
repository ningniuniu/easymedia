/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: hertz.wang hertz.wong@rock-chips.com
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

namespace rkmedia {

DECLARE_REFLECTOR(VideoEncoder, VideoEncoderFactory, VideoEncoderReflector)
DECLARE_FACTORY(VideoEncoder, VideoEncoderFactory)

class Encoder {
public:
  virtual bool InitConfig(MediaConfig &cfg) = 0;
};

class VideoEncoder : public Codec, public Encoder {
public:
  static const uint32_t kQPChange = (1 << 0);
  static const uint32_t kFrameRateChange = (1 << 1);
  static const uint32_t kBitRateChange = (1 << 2);
  static const uint32_t kForceIdrFrame = (1 << 3);

  void RequestChange(uint32_t change, int value);
  bool HasChangeReq() { return !change_list.empty(); }
  std::pair<uint32_t, int> PeekChange();

  virtual bool InitConfig(MediaConfig &cfg) override;

private:
  std::mutex change_mtx;
  std::list<std::pair<uint32_t, int>> change_list;
};

} // namespace rkmedia

#endif

#endif // RKMEDIA_ENCODER_H_
