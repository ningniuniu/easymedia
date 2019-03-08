/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_MUXER_H_
#define RKMEDIA_MUXER_H_

#include "media_config.h"
#include "media_reflector.h"
#include "stream.h"

namespace rkmedia {

DECLARE_FACTORY(Muxer)

// usage: REFLECTOR(Muxer)::Create<T>(demuxname, param)
// T must be the final class type exposed to user
DECLARE_REFLECTOR(Muxer)

#define DEFINE_MUXER_FACTORY(REAL_PRODUCT, FINAL_EXPOSE_PRODUCT)               \
  DEFINE_MEDIA_CHILD_FACTORY(REAL_PRODUCT, REAL_PRODUCT::GetMuxName(),         \
                             FINAL_EXPOSE_PRODUCT, Muxer)                      \
  DEFINE_MEDIA_CHILD_FACTORY_EXTRA(REAL_PRODUCT)                               \
  DEFINE_MEDIA_NEWINIT_PRODUCT(REAL_PRODUCT, FINAL_EXPOSE_PRODUCT)

#define DEFINE_COMMON_MUXER_FACTORY(REAL_PRODUCT)                              \
  DEFINE_MUXER_FACTORY(REAL_PRODUCT, Muxer)

class MediaBuffer;
class Encoder;
class Muxer {
public:
  Muxer(const char *param);
  virtual ~Muxer() = default;
  static const char *GetMuxName() { return nullptr; }

  virtual bool Init() = 0;
  virtual bool IncludeEncoder() { return false; }
  // create a new muxer stream by encoder, return the stream number
  virtual bool NewMuxerStream(std::shared_ptr<Encoder> enc, int &stream_no) = 0;
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

} // namespace rkmedia

#endif // #ifndef RKMEDIA_MUXER_H_
