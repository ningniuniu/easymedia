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

#include "buffer.h"
#include "demuxer.h"

static int free_memory(void *buffer) {
  assert(buffer);
  free(buffer);
  return 0;
}

static char optstr[] = "?i:o:f:c:r:";

int main(int argc, char **argv) {
  int c;
  std::string input_path;
  std::string output_path;
  std::string input_format;
  int input_channels = 0;
  int input_sample_rate = 0;

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'i':
      input_path = optarg;
      printf("input path: %s\n", input_path.c_str());
      break;
    case 'o':
      output_path = optarg;
      printf("output file path: %s\n", output_path.c_str());
      break;
    case 'f':
      input_format = optarg;
      printf("input format: %s\n", input_format.c_str());
      break;
    case 'c':
      input_channels = atoi(optarg);
      printf("input_channels: %d\n", input_channels);
      break;
    case 'r':
      input_sample_rate = atoi(optarg);
      printf("input_sample_rate: %d\n", input_sample_rate);
      break;
    case '?':
    default:
      printf("usage example: \n");
      printf("rkogg_encode_test -f s16le -c 2 -r 48000 -i input.pcm -o output.ogg\n"
             "rkogg_encode_test -f s16le -c 2 -r 48000 -i alsa:default -o output.ogg\n");
      break;
    }
  }
  if (input_path.empty() || output_path.empty() || input_format.empty() ||
      input_channels <= 0 || input_sample_rate <= 0)
    exit(EXIT_FAILURE);

  std::string alsa_device;
  if (input_path.find("alsa:") == 0) {
    alsa_device = input_path.substr(input_path.find(':') + 1);
    assert(!alsa_device.empty());
  } else {
  }

  LOG("alsa_device: %s\n", alsa_device.c_str());

  MediaConfig pcm_config;
  AudioConfig &aud_cfg = pcm_config.aud_cfg;
  SampleInfo &sample_info = aud_cfg.sample_info;
  const char *ifmt = input_format.c_str();
  if (!strcasecmp(ifmt, "u8")) {
    sample_info.fmt = SAMPLE_FMT_U8;
  } else if (!strcasecmp(ifmt, "s16le")) {
    sample_info.fmt = SAMPLE_FMT_S16;
  } else if (!strcasecmp(ifmt, "s32le")) {
    sample_info.fmt = SAMPLE_FMT_S32;
  } else {
    fprintf(stderr, "%s is unsupported now\n", ifmt);
    exit(EXIT_FAILURE);
  }
  sample_info.channels = input_channels;
  sample_info.sample_rate = input_sample_rate;
  sample_info.frames = 0;

  rkmedia::REFLECTOR(Stream)::DumpFactories();

  std::string stream_name;
  std::string params;
  std::shared_ptr<rkmedia::Stream> in_stream;
  if (!alsa_device.empty()) {
    stream_name = "alsa_capture_stream";
    std::string fmt_str = SampleFormatToString(sample_info.fmt);
    std::string rule = "output_data_type=" + fmt_str + "\n";
    if (!rkmedia::REFLECTOR(Stream)::IsMatch(stream_name.c_str(),
                                             rule.c_str())) {
      fprintf(stderr, "unsupport data type\n");
      exit(EXIT_FAILURE);
    }
    params = "device=";
    params += alsa_device + "\n";
    params += "format=" + fmt_str + "\n";
    params += "channels=" + std::to_string(sample_info.channels) + "\n";
    params += "sample_rate=" + std::to_string(sample_info.sample_rate) + "\n";
    LOG("params:\n%s\n", params.c_str());
  } else if (rkmedia::string_end_withs(input_path, ".pcm")) {
    stream_name = "file_read_stream";
    params = "path=";
    params += input_path + "\n";
    params += "mode=re\n"; // read and close-on-exec
  } else {
    assert(0);
  }

  in_stream = rkmedia::REFLECTOR(Stream)::Create<rkmedia::Stream>(
      stream_name.c_str(), params.c_str());
  if (!in_stream) {
    fprintf(stderr, "Create stream %s failed\n", stream_name.c_str());
    exit(EXIT_FAILURE);
  }

  stream_name = "file_write_stream";
  params = "path=";
  params += output_path + "\n";
  params += "mode=we\n"; // write and close-on-exec
  std::shared_ptr<rkmedia::Stream> out_stream = rkmedia::REFLECTOR(
      Stream)::Create<rkmedia::Stream>(stream_name.c_str(), params.c_str());
  if (!out_stream) {
    fprintf(stderr, "Create stream %s failed\n", stream_name.c_str());
    exit(EXIT_FAILURE);
  }

  sample_info.frames = 1024;
  int buffer_size = GetFrameSize(sample_info) * sample_info.frames;
  void *ptr = malloc(buffer_size);
  assert(ptr);
  std::shared_ptr<rkmedia::SampleBuffer> sample_buffer =
      std::make_shared<rkmedia::SampleBuffer>(
          rkmedia::MediaBuffer(ptr, buffer_size, -1, ptr, free_memory),
          sample_info);
  assert(sample_buffer);

  while (1) {
    if (in_stream->Eof())
      break;
    size_t read_size =
        in_stream->Read(sample_buffer->GetPtr(), sample_buffer->GetFrameSize(),
                        sample_buffer->GetFrames());
    if (!read_size && errno != EAGAIN) {
      exit(EXIT_FAILURE); // fatal error
    }

    out_stream->Write(
        sample_buffer->GetPtr(), sample_buffer->GetFrameSize(),
        read_size / sample_buffer->GetFrameSize()); // TODO: check the ret value
  }

  if (in_stream) {
    in_stream->Close();
    in_stream.reset();
  }

  if (out_stream) {
    out_stream->Close();
    out_stream.reset();
  }

  return 0;
}
