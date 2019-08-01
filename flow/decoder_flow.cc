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

#include <assert.h>

#include "buffer.h"
#include "decoder.h"
#include "flow.h"
#include "image.h"
#include "key_string.h"

namespace easymedia {

static bool do_decode(Flow *f, MediaBufferVector &input_vector);
class VideoDecoderFlow : public Flow {
public:
  VideoDecoderFlow(const char *param);
  virtual ~VideoDecoderFlow() { StopAllThread(); }
  static const char *GetFlowName() { return "video_dec"; }

private:
  std::shared_ptr<Decoder> decoder;
  bool support_async;
  Model thread_model;
  std::vector<std::shared_ptr<MediaBuffer>> out_buffers;
  size_t out_index;

  friend bool do_decode(Flow *f, MediaBufferVector &input_vector);
};

VideoDecoderFlow::VideoDecoderFlow(const char *param)
    : support_async(true), thread_model(Model::NONE), out_index(0) {
  std::list<std::string> separate_list;
  std::map<std::string, std::string> params;
  if (!ParseWrapFlowParams(param, params, separate_list)) {
    SetError(-EINVAL);
    return;
  }
  std::string &name = params[KEY_NAME];
  const char *decoder_name = name.c_str();
  const std::string &decode_param = separate_list.back();
  decoder = REFLECTOR(Decoder)::Create<VideoDecoder>(decoder_name,
                                                     decode_param.c_str());
  if (!decoder) {
    LOG("Create decoder %s failed\n", decoder_name);
    SetError(-EINVAL);
    return;
  }
  SlotMap sm;
  int input_maxcachenum = 2;
  ParseParamToSlotMap(params, sm, input_maxcachenum);
  if (sm.thread_model == Model::NONE)
    sm.thread_model = Model::ASYNCCOMMON;
  thread_model = sm.thread_model;
  if (sm.mode_when_full == InputMode::NONE)
    sm.mode_when_full = InputMode::BLOCKING;
  sm.input_slots.push_back(0);
  sm.input_maxcachenum.push_back(input_maxcachenum);
  sm.output_slots.push_back(0);
  sm.process = do_decode;
  if (!InstallSlotMap(sm, name, -1)) {
    LOG("Fail to InstallSlotMap, %s\n", decoder_name);
    SetError(-EINVAL);
    return;
  }
  if (decoder->SendInput(nullptr) < 0 && errno == ENOSYS)
    support_async = false;
}

bool do_decode(Flow *f, MediaBufferVector &input_vector) {
  VideoDecoderFlow *flow = static_cast<VideoDecoderFlow *>(f);
  auto decoder = flow->decoder;
  auto &in = input_vector[0];
  if (!in)
    return false;
  bool ret = false;
  std::shared_ptr<MediaBuffer> output;
  if (flow->support_async) {
    int send_ret = 0;
    do {
      send_ret = decoder->SendInput(in);
      if (send_ret != -EAGAIN)
        break;
      msleep(5);
    } while (true);
    if (send_ret)
      return false;
    do {
      output = decoder->FetchOutput();
      if (!output)
        break;
      if (flow->SetOutput(output, 0))
        ret = true;
    } while (true);
  } else {
    output = std::make_shared<ImageBuffer>();
    if (decoder->Process(in, output))
      return false;
    ret = flow->SetOutput(output, 0);
  }
  return ret;
}

DEFINE_FLOW_FACTORY(VideoDecoderFlow, Flow)
// TODO!
const char *FACTORY(VideoDecoderFlow)::ExpectedInputDataType() { return ""; }
// TODO!
const char *FACTORY(VideoDecoderFlow)::OutPutDataType() { return ""; }

} // namespace easymedia
