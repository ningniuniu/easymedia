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

#ifndef EASYMEDIA_MEDIA_CONFIG_H_
#define EASYMEDIA_MEDIA_CONFIG_H_

#include "image.h"
#include "media_type.h"
#include "sound.h"

typedef struct {
  ImageInfo image_info;
  int qp_init; // h264 : 0 - 48, higher value means higher compress
               //        but lower quality
               // jpeg (quantization coefficient): 1 - 10,
               // higher value means lower compress but higher quality,
               // contrary to h264
} ImageConfig;

typedef struct {
  ImageConfig image_cfg;
  int qp_step;
  int qp_min;
  int qp_max;
  int bit_rate;
  int frame_rate;
  int level;
  int gop_size;
  int profile;
  // quality - quality parameter
  //    (extra CQP level means special constant-qp (CQP) mode)
  //    (extra AQ_ONLY means special aq only mode)
  // "worst", "worse", "medium", "better", "best", "cqp", "aq_only"
  const char *rc_quality;
  // rc_mode - rate control mode
  // "vbr", "cbr"
  const char *rc_mode;
} VideoConfig;

typedef struct {
  SampleInfo sample_info;
  // uint64_t channel_layout;
  int bit_rate;
  float quality; // vorbis: 0.0 ~ 1.0;
} AudioConfig;

typedef struct {
  union {
    VideoConfig vid_cfg;
    ImageConfig img_cfg;
    AudioConfig aud_cfg;
  };
  Type type;
} MediaConfig;

#include <map>

namespace easymedia {
extern const char *rc_quality_strings[7];
extern const char *rc_mode_strings[2];
bool ParseMediaConfigFromMap(std::map<std::string, std::string> &params,
                             MediaConfig &mc);
_API std::string to_param_string(const ImageConfig &img_cfg);
_API std::string to_param_string(const VideoConfig &vid_cfg);
_API std::string to_param_string(const AudioConfig &aud_cfg);
_API std::string to_param_string(const MediaConfig &mc,
                                 const std::string &out_type);
} // namespace easymedia

#endif // #ifndef EASYMEDIA_MEDIA_CONFIG_H_
