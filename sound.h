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
