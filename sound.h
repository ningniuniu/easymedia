/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_SOUND_H_
#define RKMEDIA_SOUND_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  SAMPLE_FMT_NONE = -1,
  SAMPLE_FMT_U8,
  SAMPLE_FMT_S16,
  SAMPLE_FMT_S32,
  SAMPLE_FMT_NB
} SampleFormat;

typedef struct {
  SampleFormat fmt;
  int channels;
  int sample_rate;
  int frames;
} SampleInfo;

#ifdef __cplusplus
}
#endif

const char *SampleFormatToString(SampleFormat fmt);
SampleFormat StringToSampleFormat(const char *fmt_str);
bool SampleInfoIsValid(const SampleInfo &sample_info);
size_t GetFrameSize(const SampleInfo &sample_info);

#include <map>
#include <string>
namespace rkmedia {
bool ParseSampleInfoFromMap(std::map<std::string, std::string> &params,
                            SampleInfo &si);
std::string to_param_string(const SampleInfo &si);
} // namespace rkmedia

#endif // #ifndef RKMEDIA_SOUND_H_
