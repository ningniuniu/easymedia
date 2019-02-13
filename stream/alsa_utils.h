/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_ALSA_UTILS_H_
#define RKMEDIA_ALSA_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <alsa/asoundlib.h>

#ifdef __cplusplus
}
#endif

#include "sound.h"

snd_pcm_format_t SampleFormatToAlsaFormat(SampleFormat fmt);
void ShowAlsaAvailableFormats(snd_pcm_t *handle, snd_pcm_hw_params_t *params);

#endif
