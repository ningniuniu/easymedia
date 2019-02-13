/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "alsa_utils.h"

#include "utils.h"

static const struct SampleFormatEntry {
  SampleFormat fmt;
  snd_pcm_format_t alsa_fmt;
} sample_format_alsa_map[] = {
    {SAMPLE_FMT_U8, SND_PCM_FORMAT_U8},
    {SAMPLE_FMT_S16, SND_PCM_FORMAT_S16_LE},
    {SAMPLE_FMT_S32, SND_PCM_FORMAT_S32_LE},
};

snd_pcm_format_t SampleFormatToAlsaFormat(SampleFormat fmt) {
  for (size_t i = 0; i < ARRAY_ELEMS(sample_format_alsa_map) - 1; i++) {
    if (fmt == sample_format_alsa_map[i].fmt)
      return sample_format_alsa_map[i].alsa_fmt;
  }
  return SND_PCM_FORMAT_UNKNOWN;
}

void ShowAlsaAvailableFormats(snd_pcm_t *handle, snd_pcm_hw_params_t *params) {
  snd_pcm_format_t format;
  LOG("Available formats:\n");
  for (int i = 0; i <= SND_PCM_FORMAT_LAST; i++) {
    format = static_cast<snd_pcm_format_t>(i);
    if (snd_pcm_hw_params_test_format(handle, params, format) == 0)
      LOG("- %s\n", snd_pcm_format_name(format));
  }
}
