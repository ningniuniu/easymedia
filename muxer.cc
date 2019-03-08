/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "muxer.h"

namespace rkmedia {

DEFINE_REFLECTOR(Muxer)
// define the base factory missing definition
const char *FACTORY(Muxer)::Parse(const char *request) {
  // request should equal demuxer_name
  return request;
}

Muxer::Muxer(const char *param _UNUSED) {}

DEFINE_PART_FINAL_EXPOSE_PRODUCT(Muxer, Muxer)
}
