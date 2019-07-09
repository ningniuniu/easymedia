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

#ifndef EASYMEDIA_DECODER_H_
#define EASYMEDIA_DECODER_H_

#include "codec.h"
#include "media_reflector.h"

namespace easymedia {

DECLARE_FACTORY(Decoder)

// usage: REFLECTOR(Decoder)::Create<T>(codecname, param)
// T must be the final class type exposed to user
DECLARE_REFLECTOR(Decoder)

#define DEFINE_DECODER_FACTORY(REAL_PRODUCT, FINAL_EXPOSE_PRODUCT)             \
  DEFINE_MEDIA_CHILD_FACTORY(REAL_PRODUCT, REAL_PRODUCT::GetCodecName(),       \
                             FINAL_EXPOSE_PRODUCT, Decoder)                    \
  DEFINE_MEDIA_CHILD_FACTORY_EXTRA(REAL_PRODUCT)                               \
  DEFINE_MEDIA_NEW_PRODUCT_BY(REAL_PRODUCT, Decoder, Init() != true)

#define DEFINE_AUDIO_DECODER_FACTORY(REAL_PRODUCT)                             \
  DEFINE_DECODER_FACTORY(REAL_PRODUCT, AudioDecoder)

class Decoder : public Codec {
public:
  virtual ~Decoder() = default;
};

class _API AudioDecoder : public Decoder {
public:
  virtual ~AudioDecoder() = default;

  DECLARE_PART_FINAL_EXPOSE_PRODUCT(Decoder)
};

#define DEFINE_VIDEO_DECODER_FACTORY(REAL_PRODUCT)                             \
  DEFINE_DECODER_FACTORY(REAL_PRODUCT, VideoDecoder)

class _API VideoDecoder : public Decoder {
public:
  virtual ~VideoDecoder() = default;

  DECLARE_PART_FINAL_EXPOSE_PRODUCT(Decoder)
};

} // namespace easymedia

#endif // #ifndef EASYMEDIA_DECODER_H_
