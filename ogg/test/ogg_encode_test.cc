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

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>

#include "buffer.h"
#include "encoder.h"
#include "muxer.h"

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
      printf("rkogg_encode_test -f s16le -c 2 -r 48000 -i input.pcm -o "
             "output.ogg\n"
             "rkogg_encode_test -f s16le -c 2 -r 48000 -i alsa:default -o "
             "output.pcm\n"
             "rkogg_encode_test -f s16le -c 2 -r 48000 -i alsa:default -o "
             "output.ogg\n");
      break;
    }
  }
  if (input_path.empty() || output_path.empty() || input_format.empty() ||
      input_channels <= 0 || input_sample_rate <= 0)
    exit(EXIT_FAILURE);

  int64_t start_time = -1;
  std::string alsa_device;
  if (input_path.find("alsa:") == 0) {
    alsa_device = input_path.substr(input_path.find(':') + 1);
    assert(!alsa_device.empty());
    start_time = rkmedia::gettimeofday();
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

  auto write_stream = out_stream;
  std::shared_ptr<rkmedia::Encoder> enc;
  std::shared_ptr<rkmedia::Muxer> mux;
  int mux_stream_no = -1;
  if (rkmedia::string_end_withs(output_path, ".ogg")) {
    rkmedia::REFLECTOR(Muxer)::DumpFactories();
    mux = rkmedia::REFLECTOR(Muxer)::Create<rkmedia::Muxer>("liboggmuxer");
    assert(mux);
    if (!mux->IncludeEncoder()) {
      rkmedia::REFLECTOR(Encoder)::DumpFactories();
      enc = rkmedia::REFLECTOR(Encoder)::Create<rkmedia::AudioEncoder>(
          "libvorbisenc");
      assert(enc);
      aud_cfg.quality = 1.0;
      if (!enc->InitConfig(pcm_config)) {
        fprintf(stderr, "init config failed\n");
        exit(EXIT_FAILURE);
      }
      if (!mux->NewMuxerStream(enc, mux_stream_no)) {
        fprintf(stderr, "NewMuxerStream failed\n");
        exit(EXIT_FAILURE);
      }
    }
    mux->SetIoStream(write_stream);
    write_stream = nullptr;
    auto header = mux->WriteHeader(mux_stream_no);
    if (!header)
      fprintf(stderr, "warning: WriteHeader return nullptr\n");
  }

  const int read_frames = 1024;
  sample_info.frames = read_frames;
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
    if (start_time > 0 && rkmedia::gettimeofday() - start_time > 10 * 1000)
      break;
    size_t read_size = in_stream->Read(
        sample_buffer->GetPtr(), sample_buffer->GetFrameSize(), read_frames);
    if (!read_size && errno != EAGAIN) {
      exit(EXIT_FAILURE); // fatal error
    }
    sample_buffer->SetFrames(read_size);
    if (mux) {
      std::shared_ptr<rkmedia::MediaBuffer> mux_output;
      if (enc) {
        if (enc->SendInput(sample_buffer)) {
          fprintf(stderr, "warning: enc SendInput failed\n");
          exit(EXIT_FAILURE);
        }
        while (true) {
          auto enc_output = enc->FetchOutput();
          if (!enc_output)
            break;
          if (enc_output->GetValidSize() > 0) {
            mux_output = mux->Write(enc_output, mux_stream_no);
            if (write_stream && mux_output)
              write_stream->Write(mux_output->GetPtr(), 1,
                                  mux_output->GetValidSize());
          }
        }
      } else {
        mux_output = mux->Write(sample_buffer, mux_stream_no);
        if (write_stream && mux_output)
          write_stream->Write(mux_output->GetPtr(), 1,
                              mux_output->GetValidSize());
      }
    } else {
      // write pcm
      write_stream->Write(
          sample_buffer->GetPtr(), sample_buffer->GetFrameSize(),
          read_size /
              sample_buffer->GetFrameSize()); // TODO: check the ret value
    }
  }

  if (mux) {
    std::shared_ptr<rkmedia::MediaBuffer> mux_output;
    sample_buffer->SetFrames(0);
    bool eof = false;
    if (enc) {
      while (!eof) {
        if (enc->SendInput(sample_buffer))
          exit(EXIT_FAILURE);
        while (true) {
          auto enc_output = enc->FetchOutput();
          if (!enc_output)
            break;
          if (enc_output->GetValidSize() > 0) {
            mux_output = mux->Write(enc_output, mux_stream_no);
            eof = enc_output->IsEOF();
            enc_output.reset();
          }
          if (write_stream && mux_output) {
            write_stream->Write(mux_output->GetPtr(), 1,
                                mux_output->GetValidSize());
            mux_output.reset();
          }
        }
      }
    } else {
      // TODO
    }
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
