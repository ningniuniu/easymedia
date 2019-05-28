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

#ifndef RKMEDIA_MPP_INC_H_
#define RKMEDIA_MPP_INC_H_

#include "image.h"
#include <mpp/mpp_frame.h>

MppFrameFormat ConvertToMppPixFmt(const PixelFormat &fmt);
PixelFormat ConvertToPixFmt(const MppFrameFormat &mfmt);
const char *MppAcceptImageFmts();

#endif // RKMEDIA_MPP_INC_H_
