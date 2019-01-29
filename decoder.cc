/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "decoder.h"

namespace rkmedia {

DEFINE_REFLECTOR(Decoder)

// define the base factory missing definition
const char *FACTORY(Decoder)::Parse(const char *request) {
  // request should equal codec_name
  return request;
}

DEFINE_PART_FINAL_EXPOSE_PRODUCT(AudioDecoder, Decoder)

} // namespace rkmedia
