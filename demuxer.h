/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_DEMUXER_H_
#define RKMEDIA_DEMUXER_H_

#include <list>

#include "media_config.h"
#include "media_reflector.h"
#include "stream.h"

namespace rkmedia {

DECLARE_FACTORY(Demuxer)

// usage: REFLECTOR(Demuxer)::Create<T>(demuxname, param)
// T must be the final class type exposed to user
DECLARE_REFLECTOR(Demuxer)

#define DEFINE_DEMUXER_FACTORY(REAL_PRODUCT, FINAL_EXPOSE_PRODUCT)             \
  DEFINE_MEDIA_CHILD_FACTORY(REAL_PRODUCT, REAL_PRODUCT::GetDemuxName(),       \
                             FINAL_EXPOSE_PRODUCT, Demuxer)                    \
  DEFINE_MEDIA_CHILD_FACTORY_EXTRA(REAL_PRODUCT)                               \
  DEFINE_MEDIA_NEW_PRODUCT(REAL_PRODUCT, FINAL_EXPOSE_PRODUCT)

class MediaBuffer;
class Demuxer {
public:
  Demuxer(const char *param);
  virtual ~Demuxer() = default;
  static const char *GetDemuxName() { return nullptr; }

  // Indicate whether the demuxer do internally decoding
  virtual bool IncludeDecoder() { return false; }
  // Demuxer set the value of MediaConfig
  virtual bool Init(std::shared_ptr<Stream> input, MediaConfig *out_cfg) = 0;
  virtual char **GetComment() { return nullptr; }
  virtual std::shared_ptr<MediaBuffer> Read(size_t request_size = 0) = 0;

public:
  double total_time; // seconds

protected:
  std::string path;

  DECLARE_PART_FINAL_EXPOSE_PRODUCT(Demuxer)
};

} // namespace rkmedia

#endif // #ifndef RKMEDIA_DEMUXER_H_
