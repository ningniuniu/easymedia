/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "demuxer.h"

#include "buffer.h"
#include "utils.h"

namespace rkmedia {

DEFINE_REFLECTOR(Demuxer)

// define the base factory missing definition
const char *FACTORY(Demuxer)::Parse(const char *request) {
  // request should equal demuxer_name
  return request;
}

Demuxer::Demuxer(const char *param) : total_time(0.0f) {
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params))
    return;
  for (auto &p : params) {
    const std::string &key = p.first;
    if (key == "path")
      path = p.second;
  }
}

DEFINE_PART_FINAL_EXPOSE_PRODUCT(Demuxer, Demuxer)

} // namespace rkmedia
