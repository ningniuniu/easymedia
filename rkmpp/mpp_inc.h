/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_MPP_INC_H_
#define RKMEDIA_MPP_INC_H_

#include "image.h"
#include <mpp/mpp_frame.h>

MppFrameFormat ConvertToMppPixFmt(const PixelFormat &fmt);
PixelFormat ConvertToPixFmt(const MppFrameFormat &mfmt);
const char *MppAcceptImageFmts();

#endif // RKMEDIA_MPP_INC_H_
