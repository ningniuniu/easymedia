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

#ifdef NDEBUG
#undef NDEBUG
#endif
#ifndef DEBUG
#define DEBUG
#endif

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <easymedia/flow.h>
#include <easymedia/key_string.h>
#include <easymedia/media_config.h>
#include <easymedia/media_type.h>
#include <easymedia/utils.h>

static bool quit = false;

static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

#define SIMPLE 1

#if SIMPLE

#include <easymedia/buffer.h>
#include <easymedia/codec.h>

#define MAX_FILE_NUM 10
static char optstr[] = "?d:p:c:u:";

int main(int argc, char **argv) {
  int c;
  std::string input_dir_path;
  int port = 554;                  // rtsp port
  std::string channel_name;        // rtsp channel name
  std::string user_name, user_pwd; // rtsp user name and password

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'd':
      input_dir_path = optarg;
      printf("input directory path: %s\n", input_dir_path.c_str());
      break;
    case 'p':
      port = atoi(optarg);
      break;
    case 'c':
      channel_name = optarg;
      break;
    case 'u': {
      char *sep = strchr(optarg, ':');
      if (!sep) {
        fprintf(stderr, "user name and password must be seperated by :\n");
        exit(EXIT_FAILURE);
      }
      *sep = 0;
      user_name = optarg;
      user_pwd = sep + 1;
    } break;
    case '?':
    default:
      printf("usage example: \n");
      printf("rtsp_server_test -d h264_frames_dir -p 554 -c main_stream -u "
             "admin:123456\n\n");
      printf("On PC: ffplay -rtsp_transport tcp -stimeout 2000000 "
             "rtsp://admin:123456@192.168.xxx.xxx/main_stream\n");
      break;
    }
  }
  if (input_dir_path.empty() || channel_name.empty())
    assert(0);
  std::list<std::shared_ptr<easymedia::MediaBuffer>> spspps;
  std::vector<std::shared_ptr<easymedia::MediaBuffer>> buffer_list;
  buffer_list.resize(MAX_FILE_NUM);
  // 1. read all file to buffer list
  int file_index = 0;
  while (file_index <= MAX_FILE_NUM) {
    std::string input_file_path;
    input_file_path.append(input_dir_path)
        .append("/")
        .append(std::to_string(file_index))
        .append(".h264_frame");
    printf("file : %s\n", input_file_path.c_str());
    FILE *f = fopen(input_file_path.c_str(), "re");
    if (!f) {
      fprintf(stderr, "open %s failed, %m\n", input_file_path.c_str());
      exit(EXIT_FAILURE);
    }
    int len = fseek(f, 0, SEEK_END);
    if (len) {
      fprintf(stderr, "fseek %s to end failed, %m\n", input_file_path.c_str());
      exit(EXIT_FAILURE);
    }
    len = ftell(f);
    assert(len > 0);
    rewind(f);
    auto buffer = easymedia::MediaBuffer::Alloc(len);
    assert(buffer);
    ssize_t read_size = fread(buffer->GetPtr(), 1, len, f);
    assert(read_size == len);
    fclose(f);
    if (file_index == 0) {
      // 0.h264_frame must be sps pps
      // As live only accept one slice one time, separate sps and pps
      spspps = easymedia::split_h264_separate((const uint8_t *)buffer->GetPtr(),
                                              len, easymedia::gettimeofday());
      file_index++;
      continue;
    }
    uint8_t *p = (uint8_t *)buffer->GetPtr();
    uint8_t nal_type = 0;
    assert(p[0] == 0 && p[1] == 0);
    if (p[2] == 0 && p[3] == 1) {
      nal_type = p[4] & 0x1f;
    } else if (p[2] == 1) {
      nal_type = p[3] & 0x1f;
    } else {
      assert(0 && "not h264 data");
    }
    if (nal_type == 5)
      buffer->SetUserFlag(easymedia::MediaBuffer::kIntra);
    buffer->SetValidSize(len);
    // buffer->SetTimeStamp(easymedia::gettimeofday()); // ms
    buffer_list[file_index - 1] = buffer;
    file_index++;
  }
  // 2. create rtsp server
  std::string flow_name = "live555_rtsp_server";
  std::string param;
  PARAM_STRING_APPEND(param, KEY_INPUTDATATYPE,
                      VIDEO_H264); // VIDEO_H264 "," AUDIO_PCM
  PARAM_STRING_APPEND(param, KEY_CHANNEL_NAME, channel_name);
  PARAM_STRING_APPEND_TO(param, KEY_PORT_NUM, port);
  if (!user_name.empty() && !user_pwd.empty()) {
    PARAM_STRING_APPEND(param, KEY_USERNAME, user_name);
    PARAM_STRING_APPEND(param, KEY_USERPASSWORD, user_pwd);
  }
  printf("\nparam :\n%s\n", param.c_str());
  auto rtsp_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  if (!rtsp_flow || errno != 0) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }
  LOG("rtsp server test initial finish\n");
  signal(SIGINT, sigterm_handler);
  // 3. send data to live555
  int loop = 99999;
  int buffer_idx = 0;
  while (loop > 0 && !quit) {
    if (buffer_idx == 0) {
      // send sps and pps
      for (auto &buf : spspps) {
        buf->SetTimeStamp(easymedia::gettimeofday());
        rtsp_flow->SendInput(buf, 0);
      }
      buffer_idx++;
    }
    auto &buf = buffer_list[buffer_idx - 1];
    if (!buf) {
      buffer_idx = 0;
      continue;
    }
    buf->SetTimeStamp(easymedia::gettimeofday());
    rtsp_flow->SendInput(buf, 0);
    buffer_idx++;
    loop++;
    if (buffer_idx >= MAX_FILE_NUM)
      buffer_idx = 0;
    easymedia::msleep(33);
  }
  LOG("rtsp server test reclaiming\n");
  rtsp_flow.reset();
  return 0;
}

#else
static char optstr[] = "?i:w:h:p:c:u:";

int main(int argc, char **argv) {
  int c;
  std::string input_path;
  std::string input_format;
  int w = 0, h = 0, port = 554;
  std::string channel_name;
  std::string user_name, user_pwd;

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'i':
      input_path = optarg;
      printf("input file path: %s\n", input_path.c_str());
      if (!easymedia::string_end_withs(input_path, "nv12")) {
        fprintf(stderr, "input file must be *.nv12");
        exit(EXIT_FAILURE);
      }
      input_format = IMAGE_NV12;
      break;
    case 'w':
      w = atoi(optarg);
      break;
    case 'h':
      h = atoi(optarg);
      break;
    case 'p':
      port = atoi(optarg);
      break;
    case 'c':
      channel_name = optarg;
      break;
    case 'u': {
      char *sep = strchr(optarg, ':');
      if (!sep) {
        fprintf(stderr, "user name and password must be seperated by :\n");
        exit(EXIT_FAILURE);
      }
      *sep = 0;
      user_name = optarg;
      user_pwd = sep + 1;
    } break;
    case '?':
    default:
      printf("usage example: \n");
      printf("rtsp_server_test -i input.nv12 -w 320 -h 240 -p 554 -c "
             "main_stream -u admin:123456\n");
      break;
    }
  }
  if (input_path.empty() || channel_name.empty())
    exit(EXIT_FAILURE);
  if (!w || !h)
    exit(EXIT_FAILURE);
  printf("width, height: %d, %d\n", w, h);

  easymedia::REFLECTOR(Flow)::DumpFactories();
  int w_align = UPALIGNTO16(w);
  int h_align = UPALIGNTO16(h);
  int fps = 30;
  ImageInfo info;
  info.pix_fmt = PIX_FMT_NV12;
  info.width = w;
  info.height = h;
  info.vir_width = w_align;
  info.vir_height = h_align;
  std::string flow_name;
  std::string param;
  PARAM_STRING_APPEND(param, KEY_PATH, input_path);
  PARAM_STRING_APPEND(param, KEY_OPEN_MODE, "re"); // read and close-on-exec
  PARAM_STRING_APPEND(param, KEY_MEM_TYPE,
                      KEY_MEM_HARDWARE); // we will assign rkmpp as encoder,
                                         // which need hardware buffer
  param += easymedia::to_param_string(info);
  PARAM_STRING_APPEND_TO(param, KEY_FPS, fps); // rhythm reading
  PARAM_STRING_APPEND_TO(param, KEY_LOOP_TIME, 99999);
  printf("\nparam 1:\n%s\n", param.c_str());
  // 1. source
  flow_name = "file_read_flow";
  auto file_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  if (!file_flow || errno != 0) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }
  // 2. encoder
  flow_name = "video_enc";
  param = "";
  PARAM_STRING_APPEND(param, KEY_CODEC_NAME, "rkmpp_h264");
  MediaConfig enc_config;
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
  vid_cfg.frame_rate = fps;
  vid_cfg.level = 52;
  vid_cfg.gop_size = fps;
  vid_cfg.profile = 100;
  // vid_cfg.rc_quality = "aq_only"; vid_cfg.rc_mode = "vbr";
  vid_cfg.rc_quality = "best";
  vid_cfg.rc_mode = "cbr";
  param.append(easymedia::to_param_string(enc_config, VIDEO_H264));
  printf("\nparam 2:\n%s\n", param.c_str());
  auto enc_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  if (!enc_flow || errno != 0) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }
  // 3. rtsp server
  flow_name = "live555_rtsp_server";
  param = "";
  PARAM_STRING_APPEND(param, KEY_INPUTDATATYPE,
                      VIDEO_H264); // VIDEO_H264 "," AUDIO_PCM
  PARAM_STRING_APPEND(param, KEY_CHANNEL_NAME, channel_name);
  PARAM_STRING_APPEND_TO(param, KEY_PORT_NUM, port);
  if (!user_name.empty() && !user_pwd.empty()) {
    PARAM_STRING_APPEND(param, KEY_USERNAME, user_name);
    PARAM_STRING_APPEND(param, KEY_USERPASSWORD, user_pwd);
  }
  printf("\nparam 3:\n%s\n", param.c_str());
  auto rtsp_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  if (!rtsp_flow || errno != 0) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }
  // 4. link above flows
  enc_flow->AddDownFlow(rtsp_flow, 0, 0);
  file_flow->AddDownFlow(
      enc_flow, 0, 0); // the source flow better place the end to add down flow
  LOG("rtsp server test initial finish\n");
  signal(SIGINT, sigterm_handler);
  while (!quit) {
    easymedia::msleep(10);
  }
  LOG("rtsp server test reclaiming\n");
  file_flow->RemoveDownFlow(enc_flow);
  file_flow.reset();
  enc_flow->RemoveDownFlow(rtsp_flow);
  enc_flow.reset();
  rtsp_flow.reset();
  return 0;
}
#endif
