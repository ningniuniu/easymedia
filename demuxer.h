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

#ifndef EASYMEDIA_DEMUXER_H_
#define EASYMEDIA_DEMUXER_H_

#include <list>

#include "media_config.h"
#include "media_reflector.h"
#include "stream.h"

namespace easymedia {

DECLARE_FACTORY(Demuxer)

// usage: REFLECTOR(Demuxer)::Create<T>(demuxname, param)
// T must be the final class type exposed to user
DECLARE_REFLECTOR(Demuxer)

#define DEFINE_DEMUXER_FACTORY(REAL_PRODUCT, FINAL_EXPOSE_PRODUCT)             \
  DEFINE_MEDIA_CHILD_FACTORY(REAL_PRODUCT, REAL_PRODUCT::GetDemuxName(),       \
                             FINAL_EXPOSE_PRODUCT, Demuxer)                    \
  DEFINE_MEDIA_CHILD_FACTORY_EXTRA(REAL_PRODUCT)                               \
  DEFINE_MEDIA_NEW_PRODUCT_BY(REAL_PRODUCT, FINAL_EXPOSE_PRODUCT,              \
                              GetError() < 0)

class MediaBuffer;
class _API Demuxer {
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

  DEFINE_ERR_GETSET()
  DECLARE_PART_FINAL_EXPOSE_PRODUCT(Demuxer)
};

} // namespace easymedia

#endif // #ifndef EASYMEDIA_DEMUXER_H_
