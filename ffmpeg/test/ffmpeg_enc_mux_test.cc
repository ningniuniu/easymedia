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

#ifdef NDEBUG
#undef NDEBUG
#endif
#ifndef DEBUG
#define DEBUG
#endif

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>

#include "buffer.h"
#include "encoder.h"
#include "key_string.h"
#include "muxer.h"

static char optstr[] = "?i:o:w:h:e:c:";

int main(int argc, char **argv) {
  int c;
  std::string input_path;
  std::string output_path;
  int w = 0, h = 0;
  std::string input_format;
  std::string output_format;
  std::string enc_codec_name = "rkmpp";

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'i':
      input_path = optarg;
      printf("input file path: %s\n", input_path.c_str());
      break;
    case 'o':
      output_path = optarg;
      printf("output file path: %s\n", output_path.c_str());
      break;
    case 'w':
      w = atoi(optarg);
      break;
    case 'h':
      h = atoi(optarg);
      break;
    case 'e': {
      char *cut = strchr(optarg, '_');
      if (!cut) {
        fprintf(stderr, "input/output format must be cut by \'_\'\n");
        exit(EXIT_FAILURE);
      }
      cut[0] = 0;
      input_format = optarg;
      output_format = cut + 1;
      if (output_format == "h264")
        output_format = VIDEO_H264;
      else if (output_format == "h265")
        output_format = VIDEO_H265;
      printf("input format: %s\n", input_format.c_str());
      printf("output format: %s\n", output_format.c_str());
    } break;
    case 'c': {
      char *cut = strchr(optarg, ':');
      if (cut) {
        cut[0] = 0;
        std::string ff = optarg;
        if (ff == "ffmpeg")
          enc_codec_name = cut + 1;
      } else {
        enc_codec_name = optarg;
      }
    } break;
    case '?':
    default:
      printf("usage example: \n");
      printf("ffmpeg_enc_mux_test -i input.yuv -o output.mp4 -w 320 -h 240 -e "
             "nv12_h264 -c rkmpp\n");
      exit(0);
    }
  }
  if (input_path.empty() || output_path.empty())
    exit(EXIT_FAILURE);
  if (!w || !h)
    exit(EXIT_FAILURE);
  if (input_format.empty() || output_format.empty())
    exit(EXIT_FAILURE);
  fprintf(stderr, "width, height: %d, %d\n", w, h);

  int input_file_fd = open(input_path.c_str(), O_RDONLY | O_CLOEXEC);
  assert(input_file_fd >= 0);
  unlink(output_path.c_str());

  PixelFormat fmt = PIX_FMT_NONE;
  if (input_format == "nv12") {
    fmt = PIX_FMT_NV12;
  } else {
    fprintf(stderr, "TO BE TESTED <%s:%s,%d>\n", __FILE__, __FUNCTION__,
            __LINE__);
    exit(EXIT_FAILURE);
  }

  // 1. encoder
  easymedia::REFLECTOR(Encoder)::DumpFactories();

  std::string param;
  PARAM_STRING_APPEND(param, KEY_OUTPUTDATATYPE, output_format);
  auto enc = easymedia::REFLECTOR(Encoder)::Create<easymedia::VideoEncoder>(
      enc_codec_name.c_str(), param.c_str());

  if (!enc) {
    fprintf(stderr, "Create encoder %s failed\n", enc_codec_name.c_str());
    exit(EXIT_FAILURE);
  }

  ImageInfo info = {fmt, w, h, UPALIGNTO16(w), UPALIGNTO16(h)};
  MediaConfig enc_config;

  if (output_format == VIDEO_H264 || output_format == VIDEO_H265) {
    VideoConfig &vid_cfg = enc_config.vid_cfg;
    ImageConfig &img_cfg = vid_cfg.image_cfg;
    img_cfg.image_info = info;
    img_cfg.qp_init = 24;
    vid_cfg.qp_step = 4;
    vid_cfg.qp_min = 12;
    vid_cfg.qp_max = 48;
    vid_cfg.bit_rate = w * h * 7;
    if (vid_cfg.bit_rate > 1000000) {
      vid_cfg.bit_rate /= 1000000;
      vid_cfg.bit_rate *= 1000000;
    }
    vid_cfg.frame_rate = 30;
    vid_cfg.level = 52;
    vid_cfg.gop_size = vid_cfg.frame_rate;
    vid_cfg.profile = 100;
    // vid_cfg.rc_quality = "aq_only"; vid_cfg.rc_mode = "vbr";
    vid_cfg.rc_quality = KEY_BEST;
    vid_cfg.rc_mode = KEY_CBR;
  }

  if (!enc->InitConfig(enc_config)) {
    fprintf(stderr, "Init config of encoder %s failed\n",
            enc_codec_name.c_str());
    exit(EXIT_FAILURE);
  }

  // 2. muxer
  easymedia::REFLECTOR(Muxer)::DumpFactories();
  param.clear();
  PARAM_STRING_APPEND(param, KEY_PATH, output_path);
  PARAM_STRING_APPEND(param, KEY_INPUTDATATYPE, output_format);
  auto mux = easymedia::REFLECTOR(Muxer)::Create<easymedia::Muxer>(
      "ffmpeg", param.c_str());

  if (!mux) {
    fprintf(stderr, "Create muxer ffmpeg failed\n");
    exit(EXIT_FAILURE);
  }

  int mux_stream_no = -1;
  if (!mux->NewMuxerStream(enc->GetConfig(), enc->GetExtraData(),
                           mux_stream_no)) {
    fprintf(stderr, "NewMuxerStream failed\n");
    exit(EXIT_FAILURE);
  }
  auto header = mux->WriteHeader(mux_stream_no);
  if (!header) {
    fprintf(stderr, "WriteHeader on stream index %d return nullptr\n",
            mux_stream_no);
    exit(EXIT_FAILURE);
  }

  size_t len = CalPixFmtSize(fmt, info.vir_width, info.vir_height);
  auto &&src_mb = easymedia::MediaBuffer::Alloc2(
      len, easymedia::MediaBuffer::MemType::MEM_HARD_WARE);
  assert(src_mb.GetSize() > 0);
  auto src_buffer = std::make_shared<easymedia::ImageBuffer>(src_mb, info);
  assert(src_buffer && src_buffer->GetSize() >= len);

  auto dst_buffer = easymedia::MediaBuffer::Alloc(
      len, easymedia::MediaBuffer::MemType::MEM_HARD_WARE);
  assert(dst_buffer && dst_buffer->GetSize() >= len);

  ssize_t read_len;
  size_t size = CalPixFmtSize(fmt, info.width, info.height);
  ;
  int index = 0;
  while (true) {
    auto start = easymedia::gettimeofday();
    read_len = read(input_file_fd, src_buffer->GetPtr(), size);
    if (read_len <= 0)
      break;
    src_buffer->SetValidSize(read_len);              // important
    src_buffer->SetUSTimeStamp(start);               // important
    dst_buffer->SetValidSize(dst_buffer->GetSize()); // important
    index++;
    if (0 != enc->Process(src_buffer, dst_buffer, nullptr)) {
      fprintf(stderr, "frame %d encode failed\n", index);
      continue;
    }
    size_t out_len = dst_buffer->GetValidSize();
    fprintf(stderr, "frame %d encoded, type %s, out %d bytes\n", index,
            dst_buffer->GetUserFlag() & easymedia::MediaBuffer::kIntra
                ? "I frame"
                : "P frame",
            (int)out_len);
    mux->Write(dst_buffer, mux_stream_no);
    auto diff = 1000000.0f / 30 + start - easymedia::gettimeofday();
    if (diff > 0)
      easymedia::usleep(diff);
  }

  dst_buffer->SetValidSize(0); // important
  dst_buffer->SetEOF(true);    // important
  mux->Write(dst_buffer, mux_stream_no);

  close(input_file_fd);
  mux = nullptr;
  enc = nullptr;

  return 0;
}
