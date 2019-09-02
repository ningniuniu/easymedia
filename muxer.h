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

#ifndef EASYMEDIA_MUXER_H_
#define EASYMEDIA_MUXER_H_

#include "media_config.h"
#include "media_reflector.h"
#include "media_type.h"
#include "stream.h"

namespace easymedia {

DECLARE_FACTORY(Muxer)

// usage: REFLECTOR(Muxer)::Create<T>(demuxname, param)
// T must be the final class type exposed to user
DECLARE_REFLECTOR(Muxer)

#define DEFINE_MUXER_FACTORY(REAL_PRODUCT, FINAL_EXPOSE_PRODUCT)               \
  DEFINE_MEDIA_CHILD_FACTORY(REAL_PRODUCT, REAL_PRODUCT::GetMuxName(),         \
                             FINAL_EXPOSE_PRODUCT, Muxer)                      \
  DEFINE_MEDIA_CHILD_FACTORY_EXTRA(REAL_PRODUCT)                               \
  DEFINE_MEDIA_NEW_PRODUCT_BY(REAL_PRODUCT, Muxer, Init() != true)

#define DEFINE_COMMON_MUXER_FACTORY(REAL_PRODUCT)                              \
  DEFINE_MUXER_FACTORY(REAL_PRODUCT, Muxer)

class MediaBuffer;
class Encoder;
class _API Muxer {
public:
  Muxer(const char *param);
  virtual ~Muxer() = default;
  static const char *GetMuxName() { return nullptr; }

  virtual bool Init() = 0;
  virtual bool IncludeEncoder() { return false; }
  // create a new muxer stream by encoder, return the stream number
  virtual bool
  NewMuxerStream(const MediaConfig &mc,
                 const std::shared_ptr<MediaBuffer> &enc_extra_data,
                 int &stream_no) = 0;
  // Some muxer has close integrated io operation, such as ffmpeg.
  // Need set into a corresponding Stream.
  // If set io stream, the following function 'Write' may always return nullptr.
  virtual bool SetIoStream(std::shared_ptr<Stream> output) {
    io_output = output;
    return true;
  }
  virtual std::shared_ptr<MediaBuffer> WriteHeader(int stream_no) = 0;
  // orig_data:
  //    If nullptr, means flush for prepare ending close.
  virtual std::shared_ptr<MediaBuffer>
  Write(std::shared_ptr<MediaBuffer> orig_data, int stream_no) = 0;

protected:
  std::shared_ptr<Stream> io_output;
  DECLARE_PART_FINAL_EXPOSE_PRODUCT(Muxer)
};

} // namespace easymedia

#endif // #ifndef EASYMEDIA_MUXER_H_
