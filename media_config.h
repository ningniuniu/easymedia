/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_MEDIA_CONFIG_H_
#define RKMEDIA_MEDIA_CONFIG_H_

#include "image.h"
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

typedef union {
  VideoConfig vid_cfg;
  ImageConfig img_cfg;
  AudioConfig aud_cfg;
} MediaConfig;

#include <map>
#include <string>

namespace rkmedia {
bool ParseMediaConfigFromMap(std::map<std::string, std::string> &params,
                             MediaConfig &mc);
std::string to_param_string(const ImageConfig &img_cfg);
std::string to_param_string(const VideoConfig &vid_cfg);
std::string to_param_string(const AudioConfig &aud_cfg);
std::string to_param_string(const MediaConfig &mc, const std::string &out_type);
} // namespace rkmedia

#endif // #ifndef RKMEDIA_MEDIA_CONFIG_H_
