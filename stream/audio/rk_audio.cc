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

#include "rk_audio.h"

#include "utils.h"

#ifdef AEC_AGC_ANR_ALGORITHM
extern "C" {
#include "rk_voice_prointerface.h"
}

#define RK_AUDIO_BUFFER_MAX_SIZE 12288

static void rk_voice_setpara(short int *pshw_params, short int shw_len) {
  short int shwcnt;

  for (shwcnt = 0; shwcnt < shw_len; shwcnt++)
    pshw_params[shwcnt] = 0;

  /* ----- 0.total ------ */
  pshw_params[0] = 16000;
  pshw_params[1] = 320;
  /* ------ 1.AEC ------ */
  pshw_params[10] = 1;
  pshw_params[11] = 320;
  pshw_params[12] = 1;
  pshw_params[13] = 0;
  pshw_params[14] = 160;
  pshw_params[15] = 16;
  pshw_params[16] = 0;
  pshw_params[17] = 320;
  pshw_params[18] = 640;
  pshw_params[19] = 16000;
  pshw_params[20] = 320;
  pshw_params[21] = 32;
  pshw_params[22] = 1;
  pshw_params[23] = 1;
  pshw_params[24] = 0;
  pshw_params[25] = 1;
  pshw_params[26] = 320;
  pshw_params[27] = 32;
  pshw_params[28] = 200;
  pshw_params[29] = 5;
  pshw_params[30] = 10;
  pshw_params[31] = 1;
  pshw_params[32] = 10;
  pshw_params[33] = 20;
  pshw_params[34] = (short int)(0.3f * (1 << 15));
  pshw_params[35] = 1;
  pshw_params[36] = 32;
  pshw_params[37] = 5;
  pshw_params[38] = 1;
  pshw_params[39] = 10;
  pshw_params[40] = 3;
  pshw_params[41] = 3;
  pshw_params[42] = (short int)(0.30f * (1 << 15));
  pshw_params[43] = (short int)(0.04f * (1 << 15));
  pshw_params[44] = (short int)(0.40f * (1 << 15));
  pshw_params[45] = (short int)(0.25f * (1 << 15));
  pshw_params[46] = (short int)(0.0313f * (1 << 15));
  pshw_params[47] = (short int)(0.0625 * (1 << 15));
  pshw_params[48] = (short int)(0.0938 * (1 << 15));
  pshw_params[49] = (short int)(0.1250 * (1 << 15));
  pshw_params[50] = (short int)(0.1563 * (1 << 15));
  pshw_params[51] = (short int)(0.1875 * (1 << 15));
  pshw_params[52] = (short int)(0.2188 * (1 << 15));
  pshw_params[53] = (short int)(0.2500 * (1 << 15));
  pshw_params[54] = (short int)(0.2813 * (1 << 15));
  pshw_params[55] = (short int)(0.3125 * (1 << 15));
  pshw_params[56] = (short int)(0.3438 * (1 << 15));
  pshw_params[57] = (short int)(0.3750 * (1 << 15));
  pshw_params[58] = (short int)(0.4063 * (1 << 15));
  pshw_params[59] = (short int)(0.4375 * (1 << 15));
  pshw_params[60] = (short int)(0.4688 * (1 << 15));
  pshw_params[61] = (short int)(0.5000 * (1 << 15));
  pshw_params[62] = (short int)(0.5313 * (1 << 15));
  pshw_params[63] = (short int)(0.5625 * (1 << 15));
  pshw_params[64] = (short int)(0.5938 * (1 << 15));
  pshw_params[65] = (short int)(0.6250 * (1 << 15));
  pshw_params[66] = (short int)(0.6563 * (1 << 15));
  pshw_params[67] = (short int)(0.6875 * (1 << 15));
  pshw_params[68] = (short int)(0.7188 * (1 << 15));
  pshw_params[69] = (short int)(0.7500 * (1 << 15));
  pshw_params[70] = (short int)(0.7813 * (1 << 15));
  pshw_params[71] = (short int)(0.8125 * (1 << 15));
  pshw_params[72] = (short int)(0.8438 * (1 << 15));
  pshw_params[73] = (short int)(0.8750 * (1 << 15));
  pshw_params[74] = (short int)(0.9063 * (1 << 15));
  pshw_params[75] = (short int)(0.9375 * (1 << 15));
  pshw_params[76] = (short int)(0.9688 * (1 << 15));
  pshw_params[77] = (short int)(1.0000 * (1 << 15));

  /* ------ 2.ANR ------ */
  pshw_params[90] = 1;
  pshw_params[91] = 16000;
  pshw_params[92] = 320;
  pshw_params[93] = 32;
  pshw_params[94] = 2;

  pshw_params[110] = 1;
  pshw_params[111] = 16000;
  pshw_params[112] = 320;
  pshw_params[113] = 32;
  pshw_params[114] = 2;

  /* ------ 3.AGC ------ */
  pshw_params[130] = 1;
  pshw_params[131] = 16000;
  pshw_params[132] = 320;
  pshw_params[133] = (short int)(6.0f * (1 << 5));
  pshw_params[134] = (short int)(-55.0f * (1 << 5));
  pshw_params[135] = (short int)(-46.0f * (1 << 5));
  pshw_params[136] = (short int)(-24.0f * (1 << 5));
  pshw_params[137] = (short int)(1.2f * (1 << 12));
  pshw_params[138] = (short int)(0.8f * (1 << 12));
  pshw_params[139] = (short int)(0.4f * (1 << 12));
  pshw_params[140] = 40;
  pshw_params[141] = 80;
  pshw_params[142] = 80;

  pshw_params[150] = 0;
  pshw_params[151] = 16000;
  pshw_params[152] = 320;
  pshw_params[153] = (short int)(6.0f * (1 << 5));
  pshw_params[154] = (short int)(-55.0f * (1 << 5));
  pshw_params[155] = (short int)(-46.0f * (1 << 5));
  pshw_params[156] = (short int)(-24.0f * (1 << 5));
  pshw_params[157] = (short int)(1.2f * (1 << 12));
  pshw_params[158] = (short int)(0.8f * (1 << 12));
  pshw_params[159] = (short int)(0.4f * (1 << 12));
  pshw_params[160] = 40;
  pshw_params[161] = 80;
  pshw_params[162] = 80;

  /* ------ 4.EQ ------ */
  pshw_params[170] = 0;
  pshw_params[171] = 320;
  pshw_params[172] = 1;
  pshw_params[173] = (short int)(1.0f * (1 << 15));

  pshw_params[330] = 0;
  pshw_params[331] = 320;
  pshw_params[332] = 1;
  pshw_params[333] = (short int)(1.0f * (1 << 15));

  /* ------ 5.CNG ------ */
  pshw_params[490] = 1;
  pshw_params[491] = 16000;
  pshw_params[492] = 320;
  pshw_params[493] = 2;
  pshw_params[494] = 10;
  pshw_params[495] = (short int)(0.92f * (1 << 15));
  pshw_params[496] = (short int)(0.3f * (1 << 15));
}

/* for before process */
static int g_capture_buffer_size = 0;
static char *g_capture_buffer = NULL;
/* for after process */
static int g_captureout_buffer_size = 0;
static char *g_captureout_buffer = NULL;

static uint8_t trans_mark = 0;

static int queue_capture_buffer(void *buffer, int bytes) {
  if ((buffer == NULL) || (bytes <= 0)) {
    LOG("queue_capture_buffer buffer error!");
    return -1;
  }
  if (g_capture_buffer_size + bytes > BUFFER_MAX_SIZE) {
    LOG("unexpected cap buffer size too big!! return!");
    return -1;
  }

  memcpy((char *)g_capture_buffer + g_capture_buffer_size, (char *)buffer,
         bytes);
  g_capture_buffer_size += bytes;
  return 0;
}

int queue_captureout_buffer(void *buffer, int bytes) {
  if ((buffer == NULL) || (bytes <= 0)) {
    LOG("queue_captureout_buffer buffer error!");
    return -1;
  }
  if (g_captureout_buffer_size + bytes > RK_AUDIO_BUFFER_MAX_SIZE) {
    LOG("unexpected cap out buffer size too big!! return!");
    return -1;
  }

  memcpy((char *)g_captureout_buffer + g_captureout_buffer_size, (char *)buffer,
         bytes);
  g_captureout_buffer_size += bytes;
  return 0;
}

bool rk_aec_agc_anr_algorithm_support() { return true; }

int rk_voice_init(const SampleInfo &sample_info, short int ashw_para[500]) {
  if (sample_info.sample_rate != 16000) {
    LOG("rk aec_agc_anr_algorithm only support sample_rate 16000.\n");
    return -1;
  }
  rk_voice_setpara(ashw_para, 500);
  trans_mark = 0;

  if (g_capture_buffer == NULL) {
    g_capture_buffer = (char *)malloc(BUFFER_MAX_SIZE);
    if (g_capture_buffer == NULL) {
      LOG("malloc g_capture_buffer error");
      return -1;
    }
  }

  if (g_captureout_buffer == NULL) {
    g_captureout_buffer = (char *)malloc(BUFFER_MAX_SIZE);
    if (g_captureout_buffer == NULL) {
      LOG("malloc g_capture_buffer error");
      return -1;
    }
  }
  VOICE_Init(ashw_para);
}

void rk_voice_handle(void *buffer, int bytes) {
  int i, j;
  int16_t *tmp1 = NULL;
  int16_t *tmp2 = NULL;
  int16_t *left = NULL;
  int16_t *right = NULL;

  /* hard code 1280bytes/320frames/16K stereo/20ms processed one at a time */
  queue_capture_buffer(buffer, bytes);

  for (i = 0; i < g_capture_buffer_size / 1280; i++) {
    int16_t tmp_short = 320;
    short tmp_buffer[tmp_short * 2];
    memset(tmp_buffer, 0x00, tmp_short * 2);

    short in[tmp_short];
    memset(in, 0x00, tmp_short);

    short ref[tmp_short];
    memset(ref, 0x00, tmp_short);

    short out[tmp_short];
    memset(out, 0x00, tmp_short);

    tmp1 = (int16_t *)g_capture_buffer + i * 320 * 2;
    left = (int16_t *)ref;
    right = (int16_t *)in;
    for (j = 0; j < tmp_short; j++) {
      *left++ = *tmp1++;
      *right++ = *tmp1++;
    }

    if (trans_mark) {
      VOICE_ProcessTx((int16_t *)in, (int16_t *)ref, (int16_t *)out, 320);
    } else {
      /* Use ProcessRx to do ANR */
      VOICE_ProcessRx((int16_t *)in, (int16_t *)out, 320);
    }

    memset(tmp_buffer, 0x00, tmp_short * 2);

    tmp2 = (int16_t *)out;
    tmp1 = (int16_t *)tmp_buffer;
    for (j = 0; j < tmp_short; j++) {
      *tmp1++ = *tmp2;
      *tmp1++ = *tmp2++;
    }

    queue_captureout_buffer(tmp_buffer, 320 * 4);
  }
  /* Samples still keep in queuePlaybackBuffer */
  g_capture_buffer_size = g_capture_buffer_size - 1280 * i;
  /* Copy the rest of the sample to the beginning of the Buffer */
  memcpy((char *)g_capture_buffer, (char *)(g_capture_buffer + i * 1280),
         g_capture_buffer_size);

  if (g_captureout_buffer_size >= bytes) {
    memcpy((char *)buffer, (char *)g_captureout_buffer, bytes);
    g_captureout_buffer_size = g_captureout_buffer_size - bytes;
    memcpy((char *)g_captureout_buffer, (char *)(g_captureout_buffer + bytes),
           g_captureout_buffer_size);
  } else {
    LOG("g_captureout_buffer_size: less than 1024 frames\n");
  }
}

void rk_voice_deinit() {
  if (g_capture_buffer) {
    free(g_capture_buffer);
    g_capture_buffer = NULL;
  }
  if (g_captureout_buffer) {
    free(g_captureout_buffer);
    g_captureout_buffer = NULL;
  }
  VOICE_Destory();
}

#else
bool rk_aec_agc_anr_algorithm_support() { return false; }
int rk_voice_init(const SampleInfo &sample_info _UNUSED,
                  short int ashw_para[500] _UNUSED) {
  return 0;
}
void rk_voice_open(short int ashw_para[500] _UNUSED) {}
void rk_voice_handle(void *buffer _UNUSED, int bytes _UNUSED) {}
void rk_voice_deinit() {}
#endif
