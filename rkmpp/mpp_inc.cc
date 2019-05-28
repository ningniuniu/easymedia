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

#include "mpp_inc.h"

#include <assert.h>

#include "media_type.h"
#include "utils.h"

MppFrameFormat ConvertToMppPixFmt(const PixelFormat &fmt) {
  // static const MppFrameFormat invalid_mpp_fmt =
  // static_cast<MppFrameFormat>(-1);
  static_assert(PIX_FMT_YUV420P == 0, "The index should greater than 0\n");
  static MppFrameFormat mpp_fmts[PIX_FMT_NB] = {
      [PIX_FMT_YUV420P] = MPP_FMT_YUV420P,
      [PIX_FMT_NV12] = MPP_FMT_YUV420SP,
      [PIX_FMT_NV21] = MPP_FMT_YUV420SP_VU,
      [PIX_FMT_YUV422P] = MPP_FMT_YUV422P,
      [PIX_FMT_NV16] = MPP_FMT_YUV422SP,
      [PIX_FMT_NV61] = MPP_FMT_YUV422SP_VU,
      [PIX_FMT_YUYV422] = MPP_FMT_YUV422_YUYV,
      [PIX_FMT_UYVY422] = MPP_FMT_YUV422_UYVY,
      [PIX_FMT_RGB565] = MPP_FMT_RGB565,
      [PIX_FMT_BGR565] = MPP_FMT_BGR565,
      [PIX_FMT_RGB888] = MPP_FMT_RGB888,
      [PIX_FMT_BGR888] = MPP_FMT_BGR888,
      [PIX_FMT_ARGB8888] = MPP_FMT_ARGB8888,
      [PIX_FMT_ABGR8888] = MPP_FMT_ABGR8888};
  assert(fmt >= 0 && fmt < PIX_FMT_NB);
  return mpp_fmts[fmt];
}

PixelFormat ConvertToPixFmt(const MppFrameFormat &mfmt) {
  switch (mfmt) {
  case MPP_FMT_YUV420P:
    return PIX_FMT_YUV420P;
  case MPP_FMT_YUV420SP:
    return PIX_FMT_NV12;
  case MPP_FMT_YUV420SP_VU:
    return PIX_FMT_NV21;
  case MPP_FMT_YUV422P:
    return PIX_FMT_YUV422P;
  case MPP_FMT_YUV422SP:
    return PIX_FMT_NV16;
  case MPP_FMT_YUV422SP_VU:
    return PIX_FMT_NV61;
  case MPP_FMT_YUV422_YUYV:
    return PIX_FMT_YUYV422;
  case MPP_FMT_YUV422_UYVY:
    return PIX_FMT_UYVY422;
  case MPP_FMT_RGB565:
    return PIX_FMT_RGB565;
  case MPP_FMT_BGR565:
    return PIX_FMT_BGR565;
  case MPP_FMT_RGB888:
    return PIX_FMT_RGB888;
  case MPP_FMT_BGR888:
    return PIX_FMT_BGR888;
  case MPP_FMT_ARGB8888:
    return PIX_FMT_ARGB8888;
  case MPP_FMT_ABGR8888:
    return PIX_FMT_ABGR8888;
  default:
    LOG("unsupport for mpp pixel fmt: %d\n", mfmt);
    return PIX_FMT_NONE;
  }
}

const char *MppAcceptImageFmts() {
  return TYPENEAR(IMAGE_YUV420P) TYPENEAR(IMAGE_NV12) TYPENEAR(IMAGE_NV21)
      TYPENEAR(IMAGE_YUV422P) TYPENEAR(IMAGE_NV16) TYPENEAR(IMAGE_NV61)
          TYPENEAR(IMAGE_YUYV422) TYPENEAR(IMAGE_UYVY422) TYPENEAR(IMAGE_RGB565)
              TYPENEAR(IMAGE_BGR565) TYPENEAR(IMAGE_RGB888)
                  TYPENEAR(IMAGE_BGR888) TYPENEAR(IMAGE_ARGB8888)
                      TYPENEAR(IMAGE_ABGR8888);
}
