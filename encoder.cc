/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "encoder.h"

// Copy from ffmpeg.
static const uint8_t *find_startcode_internal(const uint8_t *p,
                                              const uint8_t *end) {
  const uint8_t *a = p + 4 - ((intptr_t)p & 3);

  for (end -= 3; p < a && p < end; p++) {
    if (p[0] == 0 && p[1] == 0 && p[2] == 1)
      return p;
  }

  for (end -= 3; p < end; p += 4) {
    uint32_t x = *(const uint32_t *)p;
    //      if ((x - 0x01000100) & (~x) & 0x80008000) // little endian
    //      if ((x - 0x00010001) & (~x) & 0x00800080) // big endian
    if ((x - 0x01010101) & (~x) & 0x80808080) { // generic
      if (p[1] == 0) {
        if (p[0] == 0 && p[2] == 1)
          return p;
        if (p[2] == 0 && p[3] == 1)
          return p + 1;
      }
      if (p[3] == 0) {
        if (p[2] == 0 && p[4] == 1)
          return p + 2;
        if (p[4] == 0 && p[5] == 1)
          return p + 3;
      }
    }
  }

  for (end += 3; p < end; p++) {
    if (p[0] == 0 && p[1] == 0 && p[2] == 1)
      return p;
  }

  return end + 3;
}

const uint8_t *find_h264_startcode(const uint8_t *p, const uint8_t *end) {
  const uint8_t *out = find_startcode_internal(p, end);
  if (p < out && out < end && !out[-1])
    out--;
  return out;
}

namespace rkmedia {

DEFINE_REFLECTOR(Encoder)

// define the base factory missing definition
const char *FACTORY(Encoder)::Parse(const char *request) {
  // request should equal codec_name
  return request;
}

void VideoEncoder::RequestChange(uint32_t change,
                                 std::shared_ptr<ParameterBuffer> value) {
  auto p = std::make_pair(change, value);
  change_mtx.lock();
  change_list.push_back(std::move(p));
  change_mtx.unlock();
}

std::pair<uint32_t, std::shared_ptr<ParameterBuffer>>
VideoEncoder::PeekChange() {
  std::lock_guard<std::mutex> _lg(change_mtx);
  if (change_list.empty())
    return std::pair<uint32_t, std::shared_ptr<ParameterBuffer>>(0, nullptr);

  std::pair<uint32_t, std::shared_ptr<ParameterBuffer>> &p =
      change_list.front();
  change_list.pop_front();
  return std::move(p);
}

bool VideoEncoder::InitConfig(const MediaConfig &cfg) {
  Codec::SetConfig(cfg);
  return true;
}

DEFINE_PART_FINAL_EXPOSE_PRODUCT(VideoEncoder, Encoder)

} // namespace rkmedia
