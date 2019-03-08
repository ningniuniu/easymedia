/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_DECODER_H_
#define RKMEDIA_DECODER_H_

#include "codec.h"
#include "media_reflector.h"

namespace rkmedia {

DECLARE_FACTORY(Decoder)

// usage: REFLECTOR(Decoder)::Create<T>(codecname, param)
// T must be the final class type exposed to user
DECLARE_REFLECTOR(Decoder)

#define DEFINE_DECODER_FACTORY(REAL_PRODUCT, FINAL_EXPOSE_PRODUCT)             \
  DEFINE_MEDIA_CHILD_FACTORY(REAL_PRODUCT, REAL_PRODUCT::GetCodecName(),       \
                             FINAL_EXPOSE_PRODUCT, Decoder)                    \
  DEFINE_MEDIA_CHILD_FACTORY_EXTRA(REAL_PRODUCT)

#define DEFINE_AUDIO_DECODER_FACTORY(REAL_PRODUCT)                             \
  DEFINE_DECODER_FACTORY(REAL_PRODUCT, AudioDecoder)

class Decoder : public Codec {
public:
  virtual ~Decoder() = default;
};

class AudioDecoder : public Decoder {
  DECLARE_PART_FINAL_EXPOSE_PRODUCT(Decoder)
};

} // namespace rkmedia

#endif // #ifndef RKMEDIA_DECODER_H_
