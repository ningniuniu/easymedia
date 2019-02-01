/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_MEDIA_TYPE_H_
#define RKMEDIA_MEDIA_TYPE_H_

// My fixed convention:
//  definition = "=", value separator = ",", definition separator = "\n"
//  for example,
//    "input_data_type=image:nv12,image:uyvy422\noutput_data_type=video:h264"

// My fixed convention:
//  type internal separator = ":", type near separator = "\n"
#define TYPENEAR(type) type "\n"

#define TYPE_NOTHING nullptr
#define TYPE_ANYTHING ""

#define IMAGE_YUV420P "image:yuv420p"
#define IMAGE_NV12 "image:nv12"
#define IMAGE_NV21 "image:nv21"
#define IMAGE_YUV422P "image:yuv422p"
#define IMAGE_NV16 "image:nv16"
#define IMAGE_NV61 "image:nv61"
#define IMAGE_YUYV422 "image:yuyv422"
#define IMAGE_UYVY422 "image:uyvy422"
#define IMAGE_RGB565 "image:rgb565"
#define IMAGE_BGR565 "image:bgr565"
#define IMAGE_RGB888 "image:rgb888"
#define IMAGE_BGR888 "image:bgr888"
#define IMAGE_ARGB8888 "image:argb8888"
#define IMAGE_ABGR8888 "image:abgr8888"

#define IMAGE_JPEG "image:jpeg"

#define VIDEO_H264 "video:h264"

#define STREAM_OGG "stream:ogg"

#define AUDIO_PCM "audio:pcm"

#endif // #ifndef RKMEDIA_MEDIA_TYPE_H_
