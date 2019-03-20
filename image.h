/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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

namespace rkmedia {
bool ParseImageInfoFromMap(std::map<std::string, std::string> &params,
                           ImageInfo &ii);
std::string to_param_string(const ImageInfo &ii);
} // namespace rkmedia

#endif // #ifndef RKMEDIA_IMAGE_H_
