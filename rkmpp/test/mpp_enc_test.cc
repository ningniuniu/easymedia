/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>

#include "encoder.h"
#include "test_utils.h"

static char optstr[] = "?i:o:w:h:f:";

int main(int argc, char **argv) {
  int c;
  std::string input_path;
  std::string output_path;
  int w = 0, h = 0;
  std::string input_format;
  std::string output_format;

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
    case 'f': {
      char *cut = strchr(optarg, '_');
      if (!cut) {
        fprintf(stderr, "input/output format must be cut by \'_\'\n");
        exit(EXIT_FAILURE);
      }
      cut[0] = 0;
      input_format = optarg;
      output_format = cut + 1;
      if (output_format == "jpg")
        output_format = "jpeg";
      printf("input format: %s\n", input_format.c_str());
      printf("output format: %s\n", output_format.c_str());
    } break;
    case '?':
    default:
      printf("usage example: \n");
      printf("rkmpp_test -i input.yuv -o output.h264 -w 320 -h 240 -f "
             "nv12_h264\n");
      printf(
          "rkmpp_test -i input.yuv -o output.jpg -w 320 -h 240 -f nv12_jpeg\n");
      break;
    }
  }
  if (input_path.empty() || output_path.empty())
    exit(EXIT_FAILURE);
  if (!w || !h)
    exit(EXIT_FAILURE);
  if (input_format.empty() || output_format.empty())
    exit(EXIT_FAILURE);
  printf("width, height: %d, %d\n", w, h);

  int input_file_fd = open(input_path.c_str(), O_RDONLY | O_CLOEXEC);
  assert(input_file_fd >= 0);
  unlink(output_path.c_str());
  int output_file_fd =
      open(output_path.c_str(), O_WRONLY | O_CREAT | O_CLOEXEC);
  assert(output_file_fd >= 0);

  PixelFormat fmt = PIX_FMT_NONE;
  if (input_format == "nv12") {
    fmt = PIX_FMT_NV12;
  } else {
    fprintf(stderr, "TO BE TESTED <%s:%s,%d>\n", __FILE__, __FUNCTION__,
            __LINE__);
    exit(EXIT_FAILURE);
  }

  rkmedia::REFLECTOR(Encoder)::DumpFactories();

  // rkmpp_h264 / rkmpp_jpeg
  std::string mpp_codec = std::string("rkmpp_") + output_format;
  auto mpp_enc = rkmedia::REFLECTOR(Encoder)::Create<rkmedia::VideoEncoder>(
      mpp_codec.c_str(), nullptr);

  if (!mpp_enc) {
    fprintf(stderr, "Create encoder %s failed\n", mpp_codec.c_str());
    exit(EXIT_FAILURE);
  }

  int num, den;
  ImageInfo info = {fmt, w, h, UPALIGNTO16(w), UPALIGNTO16(h)};
  MediaConfig enc_config;

  if (output_format == "h264") {
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
    vid_cfg.rc_quality = "best";
    vid_cfg.rc_mode = "cbr";
  } else if (output_format == "jpeg") {
    ImageConfig &img_cfg = enc_config.img_cfg;
    img_cfg.image_info = info;
    img_cfg.qp_init = 10;
  }

  if (!mpp_enc->InitConfig(enc_config)) {
    fprintf(stderr, "Init config of encoder %s failed\n", mpp_codec.c_str());
    exit(EXIT_FAILURE);
  }

  GetPixFmtNumDen(fmt, num, den);
  auto src_buffer = alloc_hw_memory(info, num, den);
  assert(src_buffer);
  auto dst_buffer = alloc_hw_memory(info, num, den);
  assert(dst_buffer);

  ssize_t read_len;
  size_t size = src_buffer->GetSize();
  int index = 0;
  while ((read_len = read(input_file_fd, src_buffer->GetPtr(), size)) > 0) {
    src_buffer->SetValidSize(size);
    dst_buffer->SetValidSize(size);
    index++;
    if (0 != mpp_enc->Process(src_buffer, dst_buffer, nullptr)) {
      fprintf(stderr, "frame %d encode failed\n", index);
      continue;
    }
    size_t out_len = dst_buffer->GetValidSize();
    fprintf(stderr, "frame %d encoded, type %s, out %d bytes\n", index,
            dst_buffer->GetUserFlag() & rkmedia::VideoEncoder::kIntraFrame
                ? "I frame"
                : "P frame",
            out_len);
    write(output_file_fd, dst_buffer->GetPtr(), out_len);
  }

  close(input_file_fd);
  close(output_file_fd);

  return 0;
}
