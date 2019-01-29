/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_SOUND_H_
#define RKMEDIA_SOUND_H_

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
  int samples;
} SampleInfo;

#ifdef __cplusplus
}
#endif

#endif // #ifndef RKMEDIA_SOUND_H_
