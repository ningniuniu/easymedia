/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "codec.h"

#include <sys/prctl.h>

#include "buffer.h"
#include "utils.h"

namespace rkmedia {

Codec::~Codec() {
  if (extra_data) {
    free(extra_data);
    extra_data_size = 0;
  }
}

bool Codec::SetExtraData(void *data, size_t size, bool realloc) {
  if (extra_data) {
    free(extra_data);
    extra_data_size = 0;
  }
  if (!realloc) {
    extra_data = data;
    extra_data_size = size;
    return true;
  }
  if (!data || size == 0)
    return false;
  extra_data = malloc(size);
  if (!extra_data) {
    LOG_NO_MEMORY();
    return false;
  }
  memcpy(extra_data, data, size);
  extra_data_size = size;

  return true;
}

bool Codec::Init() { return false; }
// int Codec::Process(std::shared_ptr<MediaBuffer> input _UNUSED,
//                    std::shared_ptr<MediaBuffer> output _UNUSED,
//                    std::shared_ptr<MediaBuffer> extra_output _UNUSED) {
//   return -1;
// }

// std::shared_ptr<MediaBuffer> Codec::GenEmptyOutPutBuffer() {
//   return std::make_shared<MediaBuffer>();
// }

} // namespace rkmedia
