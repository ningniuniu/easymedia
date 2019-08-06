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

#include "encoder.h"

namespace easymedia {

DEFINE_REFLECTOR(Encoder)

// request should equal codec_name
DEFINE_FACTORY_COMMON_PARSE(Encoder)

bool Encoder::InitConfig(const MediaConfig &cfg) {
  Codec::SetConfig(cfg);
  return true;
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
  if (change_list.empty()) {
    static auto empty =
        std::pair<uint32_t, std::shared_ptr<ParameterBuffer>>(0, nullptr);
    return empty;
  }
  std::pair<uint32_t, std::shared_ptr<ParameterBuffer>> &p =
      change_list.front();
  change_list.pop_front();
  return std::move(p);
}

DEFINE_PART_FINAL_EXPOSE_PRODUCT(VideoEncoder, Encoder)
DEFINE_PART_FINAL_EXPOSE_PRODUCT(AudioEncoder, Encoder)

} // namespace easymedia
