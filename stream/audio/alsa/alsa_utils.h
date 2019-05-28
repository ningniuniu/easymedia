/*
 * Copyright (C) 2017 Hertz Wang wangh@rock-chips.com
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
 * Commercial non-GPL licensing of this software is possible.
 * For more info contact Rockchip Electronics Co., Ltd.
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

#include <map>
#include <string>

#include "sound.h"

snd_pcm_format_t SampleFormatToAlsaFormat(SampleFormat fmt);
void ShowAlsaAvailableFormats(snd_pcm_t *handle, snd_pcm_hw_params_t *params);
int ParseAlsaParams(const char *param,
                    std::map<std::string, std::string> &params,
                    std::string &device, SampleInfo &sample_info);

snd_pcm_t *AlsaCommonOpenSetHwParams(const char *device,
                                     snd_pcm_stream_t stream, int mode,
                                     SampleInfo &sample_info,
                                     snd_pcm_hw_params_t *hwparams);

#endif
