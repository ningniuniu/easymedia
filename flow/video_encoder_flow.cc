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

#include "encoder.h"
#include "flow.h"

#include "buffer.h"
#include "media_type.h"

namespace easymedia {

static bool encode(Flow *f, MediaBufferVector &input_vector);

class VideoEncoderFlow : public Flow {
public:
  VideoEncoderFlow(const char *param);
  virtual ~VideoEncoderFlow() {
    AutoPrintLine apl(__func__);
    StopAllThread();
  }
  static const char *GetFlowName() { return "video_enc"; }

private:
  std::shared_ptr<VideoEncoder> enc;
  bool extra_output;
  std::list<std::shared_ptr<MediaBuffer>> extra_buffer_list;

  friend bool encode(Flow *f, MediaBufferVector &input_vector);
};

bool encode(Flow *f, MediaBufferVector &input_vector) {
  VideoEncoderFlow *vf = (VideoEncoderFlow *)f;
  std::shared_ptr<VideoEncoder> enc = vf->enc;
  std::shared_ptr<MediaBuffer> &src = input_vector[0];
  std::shared_ptr<MediaBuffer> dst, extra_dst;
  dst = std::make_shared<MediaBuffer>(); // TODO: buffer pool
  if (!dst) {
    LOG_NO_MEMORY();
    return false;
  }
  if (vf->extra_output) {
    extra_dst = std::make_shared<MediaBuffer>();
    if (!extra_dst) {
      LOG_NO_MEMORY();
      return false;
    }
  }
  if (0 != enc->Process(src, dst, extra_dst)) {
    LOG("encoder failed\n");
    return false;
  }
  vf->SetOutput(dst, 0);
  if (vf->extra_output)
    vf->SetOutput(extra_dst, 1);

  return true;
}

VideoEncoderFlow::VideoEncoderFlow(const char *param) : extra_output(false) {
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params)) {
    errno = EINVAL;
    return;
  }
  std::string &codec_name = params[KEY_CODEC_NAME];
  if (codec_name.empty()) {
    LOG("missing %s\n", KEY_CODEC_NAME);
    errno = EINVAL;
    return;
  }
  const char *ccodec_name = codec_name.c_str();
  // check input/output type
  std::string rule;
  std::string value;
  CHECK_EMPTY_SETERRNO(value, params, KEY_INPUTDATATYPE, EINVAL)
  PARAM_STRING_APPEND(rule, KEY_INPUTDATATYPE, value);
  CHECK_EMPTY_SETERRNO(value, params, KEY_OUTPUTDATATYPE, EINVAL)
  PARAM_STRING_APPEND(rule, KEY_OUTPUTDATATYPE, value);
  if (!easymedia::REFLECTOR(Encoder)::IsMatch(ccodec_name, rule.c_str())) {
    LOG("Unsupport for video encoder %s : [%s]\n", ccodec_name, rule.c_str());
    errno = EINVAL;
    return;
  }

  MediaConfig mc;
  if (!ParseMediaConfigFromMap(params, mc)) {
    errno = EINVAL;
    return;
  }

  std::string &codec_param = params[KEY_CODEC_PARAM];
  auto encoder = easymedia::REFLECTOR(Encoder)::Create<easymedia::VideoEncoder>(
      ccodec_name, codec_param.empty() ? nullptr : codec_param.c_str());
  if (!encoder) {
    LOG("Fail to create video encoder %s<%s>\n", ccodec_name,
        codec_param.c_str());
    errno = EINVAL;
    return;
  }

  if (!encoder->InitConfig(mc)) {
    LOG("Fail to init config, %s\n", ccodec_name);
    errno = EINVAL;
    return;
  }

  void *extra_data = nullptr;
  size_t extra_data_size = 0;
  encoder->GetExtraData(extra_data, extra_data_size);
  // TODO: if not h264
  if (extra_data && extra_data_size > 0)
    extra_buffer_list = split_h264_separate((const uint8_t *)extra_data,
                                            extra_data_size, gettimeofday());

  enc = encoder;

  SlotMap sm;
  sm.input_slots.push_back(0);
  sm.output_slots.push_back(0);
  if (params["extra_output"] == "y") {
    extra_output = true;
    sm.output_slots.push_back(1);
  }
  sm.process = encode;
  sm.thread_model = Model::ASYNCCOMMON;
  sm.mode_when_full = InputMode::DROPFRONT;
  sm.input_maxcachenum.push_back(3);
  if (!InstallSlotMap(sm, codec_name, 40)) {
    LOG("Fail to InstallSlotMap, %s\n", ccodec_name);
    errno = EINVAL;
    return;
  }
  for (auto &extra_buffer : extra_buffer_list) {
    assert(extra_buffer->GetUserFlag() & MediaBuffer::kExtraIntra);
    SetOutput(extra_buffer, 0);
    if (extra_output) {
      std::shared_ptr<MediaBuffer> nullbuffer;
      SetOutput(nullbuffer, 1);
    }
  }
  errno = 0;
}

DEFINE_FLOW_FACTORY(VideoEncoderFlow, Flow)
// type depends on encoder
const char *FACTORY(VideoEncoderFlow)::ExpectedInputDataType() { return ""; }
const char *FACTORY(VideoEncoderFlow)::OutPutDataType() { return ""; }

} // namespace easymedia
