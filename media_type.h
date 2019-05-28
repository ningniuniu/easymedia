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

#ifndef RKMEDIA_MEDIA_TYPE_H_
#define RKMEDIA_MEDIA_TYPE_H_

enum class Type { None = -1, Audio = 0, Image, Video, Text };

// My fixed convention:
//  definition = "=", value separator = ",", definition separator = "\n"
//  for example,
//    "input_data_type=image:nv12,image:uyvy422\noutput_data_type=video:h264"

// My fixed convention:
//  type internal separator = ":", type near separator = "\n"
#define TYPENEAR(type) type "\n"

#define TYPE_NOTHING nullptr
#define TYPE_ANYTHING ""

#define IMAGE_PREFIX "image:"
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

#define VIDEO_PREFIX "video:"
#define VIDEO_H264 "video:h264"

#define AUDIO_PREFIX "audio:"
#define AUDIO_VORBIS "audio:vorbis"

#define AUDIO_PCM_U8 "audio:pcm_u8"
#define AUDIO_PCM_S16 "audio:pcm_s16"
#define AUDIO_PCM_S32 "audio:pcm_s32"

#define AUDIO_PCM                                                              \
  TYPENEAR(AUDIO_PCM_U8) TYPENEAR(AUDIO_PCM_S16) TYPENEAR(AUDIO_PCM_S32)

#define STREAM_OGG "stream:ogg"

#define STREAM_FILE "stream:file"

#endif // #ifndef RKMEDIA_MEDIA_TYPE_H_
