/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_RK_AUDIO_H_
#define RKMEDIA_RK_AUDIO_H_

#include <cstddef>

#include "sound.h"

// This file contains rk particular audio functions
inline bool rk_aec_agc_anr_algorithm_support();
int rk_voice_init(const SampleInfo &sample_info, short int ashw_para[500]);
void rk_voice_handle(void *buffer, int bytes);
void rk_voice_deinit();

#endif // RKMEDIA_RK_AUDIO_H_