/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "image.h"

#include "utils.h"

void GetPixFmtNumDen(const PixelFormat &fmt, int &num, int &den) {
  num = 0;
  den = 1;
  switch (fmt) {
  case PIX_FMT_YUV420P:
  case PIX_FMT_NV12:
  case PIX_FMT_NV21:
    num = 3;
    den = 2;
    break;
  case PIX_FMT_YUV422P:
  case PIX_FMT_NV16:
  case PIX_FMT_NV61:
  case PIX_FMT_YVYU422:
  case PIX_FMT_UYVY422:
  case PIX_FMT_RGB565:
  case PIX_FMT_BGR565:
    num = 2;
    break;
  case PIX_FMT_RGB888:
  case PIX_FMT_BGR888:
    num = 3;
    break;
  case PIX_FMT_ARGB8888:
  case PIX_FMT_ABGR8888:
    num = 4;
    break;
  default:
    LOG("unsupport for pixel fmt: %d\n", fmt);
  }
}
