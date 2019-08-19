/*
 * Copyright (C) 2019 Hertz Wang 1989wanghang@163.com
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

#ifndef EASYMEDIA_FFMPEG_UTILS_H_
#define EASYMEDIA_FFMPEG_UTILS_H_

extern "C" {
#include <libavformat/avformat.h>
}

#include "image.h"
#include "sound.h"

namespace easymedia {

enum AVPixelFormat PixFmtToAVPixFmt(PixelFormat fmt);
enum AVCodecID PixFmtToAVCodecID(PixelFormat fmt);

enum AVCodecID SampleFmtToAVCodecID(SampleFormat fmt);
enum AVSampleFormat SampleFmtToAVSamFmt(SampleFormat sfmt);

void PrintAVError(int err, const char *log, const char *mark);

} // namespace easymedia

#endif // #ifndef EASYMEDIA_FFMPEG_UTILS_H_
