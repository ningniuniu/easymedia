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

#ifndef EASYMEDIA_RK_AUDIO_H_
#define EASYMEDIA_RK_AUDIO_H_

#include <cstddef>

#include "sound.h"

// This file contains rk particular audio functions
inline bool rk_aec_agc_anr_algorithm_support();
int rk_voice_init(const SampleInfo &sample_info, short int ashw_para[500]);
void rk_voice_handle(void *buffer, int bytes);
void rk_voice_deinit();

#endif // EASYMEDIA_RK_AUDIO_H_