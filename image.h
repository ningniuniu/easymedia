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

#ifndef RKMEDIA_IMAGE_H_
#define RKMEDIA_IMAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  PIX_FMT_NONE = -1,
  PIX_FMT_YUV420P,
  PIX_FMT_NV12,
  PIX_FMT_NV21,
  PIX_FMT_YUV422P,
  PIX_FMT_NV16,
  PIX_FMT_NV61,
  PIX_FMT_YUYV422,
  PIX_FMT_UYVY422,
  PIX_FMT_RGB565,
  PIX_FMT_BGR565,
  PIX_FMT_RGB888,
  PIX_FMT_BGR888,
  PIX_FMT_ARGB8888,
  PIX_FMT_ABGR8888,
  PIX_FMT_NB
} PixelFormat;

typedef struct {
  PixelFormat pix_fmt;
  int width;      // valid pixel width
  int height;     // valid pixel height
  int vir_width;  // stride width, same to buffer_width, must greater than
                  // width, often set vir_width=(width+15)&(~15)
  int vir_height; // stride height, same to buffer_height, must greater than
                  // height, often set vir_height=(height+15)&(~15)
} ImageInfo;

#ifdef __cplusplus
}
#endif

void GetPixFmtNumDen(const PixelFormat &fmt, int &num, int &den);
PixelFormat GetPixFmtByString(const char *type);
const char *PixFmtToString(PixelFormat fmt);

#include <map>
#include <string>

namespace easymedia {
bool ParseImageInfoFromMap(std::map<std::string, std::string> &params,
                           ImageInfo &ii);
std::string to_param_string(const ImageInfo &ii);
} // namespace easymedia

#endif // #ifndef RKMEDIA_IMAGE_H_
