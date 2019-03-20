/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "encoder.h"

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

bool AudioEncoder::InitConfig(const MediaConfig &cfg) {
  Codec::SetConfig(cfg);
  return true;
}

DEFINE_PART_FINAL_EXPOSE_PRODUCT(AudioEncoder, Encoder)

} // namespace rkmedia
