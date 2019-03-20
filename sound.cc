/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "sound.h"

#include <string.h>

#include "key_string.h"
#include "media_type.h"
#include "utils.h"

static const struct SampleFormatEntry {
  SampleFormat fmt;
  const char *fmt_str;
} sample_format_string_map[] = {
    {SAMPLE_FMT_U8, AUDIO_PCM_U8},
    {SAMPLE_FMT_S16, AUDIO_PCM_S16},
    {SAMPLE_FMT_S32, AUDIO_PCM_S32},
};

const char *SampleFormatToString(SampleFormat fmt) {
  for (size_t i = 0; i < ARRAY_ELEMS(sample_format_string_map) - 1; i++) {
    if (fmt == sample_format_string_map[i].fmt)
      return sample_format_string_map[i].fmt_str;
  }
  return nullptr;
}

SampleFormat StringToSampleFormat(const char *fmt_str) {
  for (size_t i = 0; i < ARRAY_ELEMS(sample_format_string_map) - 1; i++) {
    if (!strcmp(sample_format_string_map[i].fmt_str, fmt_str))
      return sample_format_string_map[i].fmt;
  }
  return SAMPLE_FMT_NONE;
}

bool SampleInfoIsValid(const SampleInfo &sample_info) {
  return (sample_info.fmt != SAMPLE_FMT_NONE) && (sample_info.channels > 0) &&
         (sample_info.sample_rate > 0);
}

size_t GetFrameSize(const SampleInfo &sample_info) {
  size_t sample_size = sample_info.channels;
  switch (sample_info.fmt) {
  case SAMPLE_FMT_U8:
    return sample_size;
  case SAMPLE_FMT_S16:
    return sample_size << 1;
  case SAMPLE_FMT_S32:
    return sample_size << 2;
  default:
    return 0;
  }
}

namespace rkmedia {

bool ParseSampleInfoFromMap(std::map<std::string, std::string> &params,
                            SampleInfo &si) {
  std::string value;
  CHECK_EMPTY(value, params, KEY_INPUTDATATYPE)
  si.fmt = StringToSampleFormat(value.c_str());
  if (si.fmt == SAMPLE_FMT_NONE) {
    LOG("unsupport sample fmt %d\n", value.c_str());
    return false;
  }
  CHECK_EMPTY(value, params, KEY_CHANNELS)
  si.channels = std::stoi(value);
  CHECK_EMPTY(value, params, KEY_SAMPLE_RATE)
  si.sample_rate = std::stoi(value);
  CHECK_EMPTY(value, params, KEY_FRAMES)
  si.frames = std::stoi(value);
  return true;
}

std::string to_param_string(const SampleInfo &si) {
  std::string s;
  const char *fmt = SampleFormatToString(si.fmt);
  if (!fmt)
    return s;
  PARAM_STRING_APPEND(s, KEY_INPUTDATATYPE, fmt);
  PARAM_STRING_APPEND_TO(s, KEY_CHANNELS, si.channels);
  PARAM_STRING_APPEND_TO(s, KEY_SAMPLE_RATE, si.sample_rate);
  PARAM_STRING_APPEND_TO(s, KEY_FRAMES, si.frames);
  return s;
}

} // namespace rkmedia
