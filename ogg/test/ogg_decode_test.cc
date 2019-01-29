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

static char optstr[] = "?i:o:";

int main(int argc, char **argv) {
  int c;
  std::string input_path;
  std::string output_path;

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
    case '?':
    default:
      printf("usage example: \n");
      printf("rkogg_decode_test -i input.ogg -o output.pcm\n");
      break;
    }
  }
  if (input_path.empty() || output_path.empty())
    exit(EXIT_FAILURE);

  unlink(output_path.c_str());
  int output_file_fd =
      open(output_path.c_str(), O_WRONLY | O_CREAT | O_CLOEXEC);
  assert(output_file_fd >= 0);

  rkmedia::REFLECTOR(Demuxer)::DumpFactories();

  // here we use fixed demuxer "oggvorbis"
  std::string codec_name("oggvorbis");
  std::string param = "path=";
  param += input_path;
  auto ogg_demuxer = rkmedia::REFLECTOR(Demuxer)::Create<rkmedia::Demuxer>(
      codec_name.c_str(), param.c_str());

  if (!ogg_demuxer) {
    fprintf(stderr, "Create demuxer %s failed\n", codec_name.c_str());
    exit(EXIT_FAILURE);
  }

  // On rockchip powerful platform, ogg audio demuxer and decoder seems
  // unnecessary to be separated.
  if (!ogg_demuxer->IncludeDecoder()) {
    LOG("ogg TODO: separate ogg demuxer and ogg decoder\n");
    exit(EXIT_FAILURE);
  }

  MediaConfig pcm_config;
  // input is file, set the stream nullptr
  if (!ogg_demuxer->Init(nullptr, &pcm_config)) {
    fprintf(stderr, "Init demuxer %s failed\n", codec_name.c_str());
    exit(EXIT_FAILURE);
  }
  char **comment = ogg_demuxer->GetComment();
  if (comment) {
    if (*comment)
      printf("ogg comment: ");
    while (*comment) {
      printf("%s\n", *comment);
      ++comment;
    }
    printf("\n");
  }
  AudioConfig &aud_cfg = pcm_config.aud_cfg;
  SampleInfo &sample_info = aud_cfg.sample_info;
  printf("channel num : %d , sample rate %d, average bit rate: %d\n",
         sample_info.channels, sample_info.sample_rate, aud_cfg.bit_rate);

  while (1) {
    auto buffer = ogg_demuxer->Read();
    if (!buffer)
      exit(EXIT_FAILURE); // fatal error
    if (buffer->IsEOF())
      break;
    assert(buffer->GetSampleFormat() != SAMPLE_FMT_NONE);
    std::shared_ptr<rkmedia::SampleBuffer> sample_buffer =
        std::static_pointer_cast<rkmedia::SampleBuffer>(buffer);
    SampleInfo &info = sample_buffer->GetSampleInfo();
    if ((info.channels != sample_info.channels) ||
        (info.sample_rate != sample_info.sample_rate)) {
      // hmm, config changed, maybe should write to another file
      fprintf(stderr, "initial config is %d %dch, but this buffer is %d %dch\n",
              sample_info.sample_rate, sample_info.channels, info.sample_rate,
              info.channels);
      continue;
    }
    write(output_file_fd, buffer->GetPtr(), buffer->GetValidSize());
  }

  close(output_file_fd);

  return 0;
}
