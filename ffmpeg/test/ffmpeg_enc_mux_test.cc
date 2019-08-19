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

#include <cmath>
#include <string>

#include "buffer.h"
#include "encoder.h"
#include "key_string.h"
#include "muxer.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

static void
fill_audio_sin_data(std::shared_ptr<easymedia::SampleBuffer> &buffer) {
  float t = 0;
  float tincr = 2 * M_PI * 10;
  uint16_t *samples = (uint16_t *)buffer->GetPtr();
  for (int i = 0; i < buffer->GetSamples(); i++) {
    samples[2 * i] = (int)(std::sin(t) * 10000);
    for (int k = 1; k < buffer->GetSampleInfo().channels; k++)
      samples[2 * i + k] = samples[2 * i];
    t += tincr;
  }
}

static char optstr[] = "?i:o:w:h:e:c:";

int main(int argc, char **argv) {
  int c;
  std::string input_path;
  std::string output_path;
  int w = 0, h = 0;
  std::string input_format;
  std::string enc_format;
  std::string enc_codec_name = "rkmpp";

  std::string aud_enc_format = AUDIO_MP2; // test mp2

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
      enc_format = cut + 1;
      if (enc_format == "h264")
        enc_format = VIDEO_H264;
      else if (enc_format == "h265")
        enc_format = VIDEO_H265;
      printf("input format: %s\n", input_format.c_str());
      printf("enc format: %s\n", enc_format.c_str());
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
  if (input_format.empty() || enc_format.empty())
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
  PARAM_STRING_APPEND(param, KEY_OUTPUTDATATYPE, enc_format);
  auto vid_enc = easymedia::REFLECTOR(Encoder)::Create<easymedia::VideoEncoder>(
      enc_codec_name.c_str(), param.c_str());

  if (!vid_enc) {
    fprintf(stderr, "Create encoder %s failed\n", enc_codec_name.c_str());
    exit(EXIT_FAILURE);
  }

  ImageInfo vid_info = {fmt, w, h, UPALIGNTO16(w), UPALIGNTO16(h)};
  MediaConfig vid_enc_config;

  int64_t vinterval_per_frame = 0;
  if (enc_format == VIDEO_H264 || enc_format == VIDEO_H265) {
    VideoConfig &vid_cfg = vid_enc_config.vid_cfg;
    ImageConfig &img_cfg = vid_cfg.image_cfg;
    img_cfg.image_info = vid_info;
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
    vinterval_per_frame = 1000000LL /* us */ / vid_cfg.frame_rate;
  } else {
    // TODO
    assert(0);
  }

  if (!vid_enc->InitConfig(vid_enc_config)) {
    fprintf(stderr, "Init config of encoder %s failed\n",
            enc_codec_name.c_str());
    exit(EXIT_FAILURE);
  }

  param.clear();
  PARAM_STRING_APPEND(param, KEY_OUTPUTDATATYPE, aud_enc_format);
  auto aud_enc = easymedia::REFLECTOR(Encoder)::Create<easymedia::AudioEncoder>(
      "ffmpeg_aud", param.c_str());
  if (!aud_enc) {
    fprintf(stderr, "Create ffmpeg_aud encoder failed\n");
    exit(EXIT_FAILURE);
  }

  // s16 2ch stereo, 1024 nb_samples
  SampleInfo aud_info = {SAMPLE_FMT_S16, 2, 48000, 1024};
  MediaConfig aud_enc_config;
  auto &ac = aud_enc_config.aud_cfg;
  ac.sample_info = aud_info;
  ac.bit_rate = 64000; // 64kbps
  if (!aud_enc->InitConfig(aud_enc_config)) {
    fprintf(stderr, "Init config of ffmpeg_aud encoder failed\n");
    exit(EXIT_FAILURE);
  }

  // 2. muxer
  easymedia::REFLECTOR(Muxer)::DumpFactories();
  param.clear();
  PARAM_STRING_APPEND(param, KEY_PATH, output_path);
  auto mux = easymedia::REFLECTOR(Muxer)::Create<easymedia::Muxer>(
      "ffmpeg", param.c_str());

  if (!mux) {
    fprintf(stderr, "Create muxer ffmpeg failed\n");
    exit(EXIT_FAILURE);
  }

  MediaConfig vid_mux_config = vid_enc->GetConfig();
  vid_mux_config.vid_cfg.image_cfg.image_info.pix_fmt =
      StringToPixFmt(enc_format.c_str());
  vid_mux_config.type = Type::Video;
  int vid_stream_no = -1;
  if (!mux->NewMuxerStream(vid_mux_config, vid_enc->GetExtraData(),
                           vid_stream_no)) {
    fprintf(stderr, "NewMuxerStream failed for video\n");
    exit(EXIT_FAILURE);
  }
  MediaConfig aud_mux_config = aud_enc->GetConfig();
  auto &audio_info = aud_mux_config.aud_cfg.sample_info;
  fprintf(stderr, "sample number=%d\n", audio_info.nb_samples);
  int aud_size = GetSampleSize(audio_info) * audio_info.nb_samples;
  auto aud_mb = easymedia::MediaBuffer::Alloc2(aud_size);
  auto aud_buffer =
      std::make_shared<easymedia::SampleBuffer>(aud_mb, audio_info);
  assert(aud_buffer && (int)aud_buffer->GetSize() >= aud_size);
  fill_audio_sin_data(aud_buffer);
  int64_t ainterval_per_frame =
      1000000LL /* us */ * audio_info.nb_samples / audio_info.sample_rate;
  audio_info.fmt = StringToSampleFmt(aud_enc_format.c_str());
  vid_mux_config.type = Type::Audio;
  int aud_stream_no = -1;
  if (!mux->NewMuxerStream(aud_mux_config, aud_enc->GetExtraData(),
                           aud_stream_no)) {
    fprintf(stderr, "NewMuxerStream failed for audio\n");
    exit(EXIT_FAILURE);
  }

  auto header = mux->WriteHeader(vid_stream_no);
  if (!header) {
    fprintf(stderr, "WriteHeader on stream index %d return nullptr\n",
            vid_stream_no);
    exit(EXIT_FAILURE);
  }
  // for ffmpeg, WriteHeader once, this call only dump info
  mux->WriteHeader(aud_stream_no);

  size_t len = CalPixFmtSize(fmt, vid_info.vir_width, vid_info.vir_height);
  auto &&src_mb = easymedia::MediaBuffer::Alloc2(
      len, easymedia::MediaBuffer::MemType::MEM_HARD_WARE);
  assert(src_mb.GetSize() > 0);
  auto src_buffer = std::make_shared<easymedia::ImageBuffer>(src_mb, vid_info);
  assert(src_buffer && src_buffer->GetSize() >= len);

  auto dst_buffer = easymedia::MediaBuffer::Alloc(
      len, easymedia::MediaBuffer::MemType::MEM_HARD_WARE);
  assert(dst_buffer && dst_buffer->GetSize() >= len);

  ssize_t read_len;
  int vid_index = 0;
  int64_t first_video_time = 0;
  int aud_index = 0;
  int aud_out_index = 0;
  int64_t first_audio_time = 0;
  while (true) {
    // video
    if (first_video_time == 0)
      first_video_time = easymedia::gettimeofday();
    read_len = read(input_file_fd, src_buffer->GetPtr(), len);
    if (read_len <= 0)
      break;
    src_buffer->SetValidSize(read_len); // important
    src_buffer->SetUSTimeStamp(first_video_time +
                               vid_index * vinterval_per_frame); // important
    dst_buffer->SetValidSize(dst_buffer->GetSize());             // important
    vid_index++;
    if (0 != vid_enc->Process(src_buffer, dst_buffer, nullptr)) {
      fprintf(stderr, "vframe %d encode failed\n", vid_index);
      continue;
    }
    size_t out_len = dst_buffer->GetValidSize();
    fprintf(stderr, "vframe %d encoded, type %s, out %d bytes\n", vid_index,
            dst_buffer->GetUserFlag() & easymedia::MediaBuffer::kIntra
                ? "I frame"
                : "P frame",
            (int)out_len);
    mux->Write(dst_buffer, vid_stream_no);
    // audio
    aud_buffer->SetValidSize(aud_size); // important
    if (first_audio_time == 0)
      first_audio_time = easymedia::gettimeofday();
    aud_buffer->SetUSTimeStamp(first_audio_time +
                               aud_index * ainterval_per_frame); // important
    int aud_ret = aud_enc->SendInput(aud_buffer);
    aud_index++;
    if (aud_ret < 0)
      fprintf(stderr, "aframe %d encode failed, ret=%d\n", aud_index, aud_ret);
    fprintf(stderr, "\naframe %d sent\n", aud_index);
    while (aud_ret >= 0) {
      auto aud_out = aud_enc->FetchOutput();
      if (!aud_out) {
        if (errno != EAGAIN)
          fprintf(stderr, "aframe %d fetch failed, ret=%d\n", aud_out_index,
                  errno);
        break;
      }
      out_len = aud_out->GetValidSize();
      if (out_len == 0)
        break;
      aud_out_index++;
      fprintf(stderr, "aframe %d encoded, out %d bytes\n\n", aud_out_index,
              (int)out_len);
      mux->Write(aud_out, aud_stream_no);
    }
  }

  // some audio encoder need flush
  aud_buffer->SetSamples(0);
  bool eof = false;
  while (!eof) {
    int aud_ret = aud_enc->SendInput(aud_buffer);
    if (aud_ret < 0)
      fprintf(stderr, "aframe encode flush failed, ret=%d\n", aud_ret);
    while (!eof) {
      auto aud_out = aud_enc->FetchOutput();
      if (!aud_out)
        break;
      eof = aud_out->IsEOF();
      auto out_len = aud_out->GetValidSize();
      if (aud_out->GetValidSize() > 0) {
        fprintf(stderr, "aframe encode flush, out %d bytes\n", (int)out_len);
        mux->Write(aud_out, aud_stream_no);
      }
    }
  }

  dst_buffer->SetValidSize(0); // important
  dst_buffer->SetEOF(true);    // important
  mux->Write(dst_buffer, vid_stream_no);

  close(input_file_fd);
  mux = nullptr;
  vid_enc = nullptr;
  aud_enc = nullptr;

  return 0;
}
