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

#include "ffmpeg_utils.h"

namespace easymedia {

static const struct PixFmtAVPFEntry {
  PixelFormat fmt;
  enum AVPixelFormat av_fmt;
} pix_fmt_av_pixfmt_map[] = {
    {PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P},
    {PIX_FMT_NV12, AV_PIX_FMT_NV12},
    {PIX_FMT_NV21, AV_PIX_FMT_NV21},
    {PIX_FMT_YUV422P, AV_PIX_FMT_YUV422P16LE},
    {PIX_FMT_NV16, AV_PIX_FMT_NV16},
    {PIX_FMT_NV61, AV_PIX_FMT_NONE},
    {PIX_FMT_YUYV422, AV_PIX_FMT_YUYV422},
    {PIX_FMT_UYVY422, AV_PIX_FMT_UYVY422},
    {PIX_FMT_RGB332, AV_PIX_FMT_RGB8},
    {PIX_FMT_RGB565, AV_PIX_FMT_RGB565LE},
    {PIX_FMT_BGR565, AV_PIX_FMT_BGR565LE},
    {PIX_FMT_RGB888, AV_PIX_FMT_RGB24},
    {PIX_FMT_BGR888, AV_PIX_FMT_BGR24},
    {PIX_FMT_ARGB8888, AV_PIX_FMT_ARGB},
    {PIX_FMT_ABGR8888, AV_PIX_FMT_ABGR},
};

enum AVPixelFormat PixFmtToAVPixFmt(PixelFormat fmt) {
  FIND_ENTRY_TARGET(fmt, pix_fmt_av_pixfmt_map, fmt, av_fmt)
  return AV_PIX_FMT_NONE;
}

static const struct PixFmtAVCodecIDEntry {
  PixelFormat fmt;
  enum AVCodecID av_codecid;
} pix_fmt_av_codecid_map[] = {
    {PIX_FMT_H264, AV_CODEC_ID_H264},
    {PIX_FMT_H265, AV_CODEC_ID_H265},
};

enum AVCodecID PixFmtToAVCodecID(PixelFormat fmt) {
  FIND_ENTRY_TARGET(fmt, pix_fmt_av_codecid_map, fmt, av_codecid)
  return AV_CODEC_ID_NONE;
}

static const struct SampleFmtAVSFEntry {
  SampleFormat fmt;
  enum AVSampleFormat av_sfmt;
} sample_fmt_av_sfmt_map[] = {
    {SAMPLE_FMT_U8, AV_SAMPLE_FMT_U8},
    {SAMPLE_FMT_S16, AV_SAMPLE_FMT_S16},
    {SAMPLE_FMT_S32, AV_SAMPLE_FMT_S32},
};

enum AVSampleFormat SampleFmtToAVSamFmt(SampleFormat sfmt) {
  FIND_ENTRY_TARGET(sfmt, sample_fmt_av_sfmt_map, fmt, av_sfmt)
  return AV_SAMPLE_FMT_NONE;
}

} // namespace easymedia