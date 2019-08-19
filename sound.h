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

#ifndef EASYMEDIA_SOUND_H_
#define EASYMEDIA_SOUND_H_

#include <stddef.h>

#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  SAMPLE_FMT_NONE = -1,
  SAMPLE_FMT_U8,
  SAMPLE_FMT_S16,
  SAMPLE_FMT_S32,
  SAMPLE_FMT_VORBIS,
  SAMPLE_FMT_AAC,
  SAMPLE_FMT_MP2,
  SAMPLE_FMT_NB
} SampleFormat;

typedef struct {
  SampleFormat fmt;
  int channels;
  int sample_rate;
  int nb_samples;
} SampleInfo;

#ifdef __cplusplus
}
#endif

_API const char *SampleFmtToString(SampleFormat fmt);
_API SampleFormat StringToSampleFmt(const char *fmt_str);
_API bool SampleInfoIsValid(const SampleInfo &sample_info);
_API size_t GetSampleSize(const SampleInfo &sample_info);

#include <map>
#include <string>
namespace easymedia {
bool ParseSampleInfoFromMap(std::map<std::string, std::string> &params,
                            SampleInfo &si);
std::string _API to_param_string(const SampleInfo &si);
} // namespace easymedia

#endif // #ifndef EASYMEDIA_SOUND_H_
