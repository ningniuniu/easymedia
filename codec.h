/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_CODEC_H_
#define RKMEDIA_CODEC_H_

#include <string.h>

#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "media_config.h"

namespace rkmedia {

class MediaBuffer;
class Codec {
public:
  Codec() : extra_data(nullptr), extra_data_size(0) {
    memset(&config, 0, sizeof(config));
  }
  virtual ~Codec() = 0;
  static const char *GetCodecName() { return nullptr; }
  MediaConfig &GetConfig() { return config; }
  void SetConfig(const MediaConfig &cfg) { config = cfg; }
  bool SetExtraData(void *data, size_t size, bool realloc = true);
  void GetExtraData(void *&data, size_t &size) {
    data = extra_data;
    size = extra_data_size;
  }

  virtual bool Init() = 0;
  // sync call, input and output must be valid
  virtual int Process(std::shared_ptr<MediaBuffer> input,
                      std::shared_ptr<MediaBuffer> output,
                      std::shared_ptr<MediaBuffer> extra_output = nullptr) = 0;

  // some codec may output many buffers with one input.
  // sync or async safe call, depends on codec.
  virtual int SendInput(std::shared_ptr<MediaBuffer> input) = 0;
  virtual std::shared_ptr<MediaBuffer> FetchOutput() = 0;

  // virtual std::shared_ptr<MediaBuffer> GenEmptyOutPutBuffer();

private:
  MediaConfig config;
  void *extra_data;
  size_t extra_data_size;
};

const uint8_t *find_h264_startcode(const uint8_t *p, const uint8_t *end);
// must be h264 data
std::list<std::shared_ptr<MediaBuffer>>
split_h264_separate(const uint8_t *buffer, size_t length, int64_t timestamp);

} // namespace rkmedia

#endif // #ifndef RKMEDIA_CODEC_H_
