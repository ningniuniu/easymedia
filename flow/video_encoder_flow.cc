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
  bool ret = vf->SetOutput(dst, 0);
  if (vf->extra_output)
    ret &= vf->SetOutput(extra_dst, 1);

  return ret;
}

VideoEncoderFlow::VideoEncoderFlow(const char *param) : extra_output(false) {
  std::list<std::string> separate_list;
  std::map<std::string, std::string> params;
  if (!ParseWrapFlowParams(param, params, separate_list)) {
    SetError(-EINVAL);
    return;
  }
  std::string &codec_name = params[KEY_NAME];
  if (codec_name.empty()) {
    LOG("missing codec name\n");
    SetError(-EINVAL);
    return;
  }
  const char *ccodec_name = codec_name.c_str();
  // check input/output type
  std::string &&rule = gen_datatype_rule(params);
  if (rule.empty()) {
    SetError(-EINVAL);
    return;
  }
  if (!REFLECTOR(Encoder)::IsMatch(ccodec_name, rule.c_str())) {
    LOG("Unsupport for video encoder %s : [%s]\n", ccodec_name, rule.c_str());
    SetError(-EINVAL);
    return;
  }

  const std::string &enc_param_str = separate_list.back();
  std::map<std::string, std::string> enc_params;
  if (!parse_media_param_map(enc_param_str.c_str(), enc_params)) {
    SetError(-EINVAL);
    return;
  }
  MediaConfig mc;
  if (!ParseMediaConfigFromMap(enc_params, mc)) {
    SetError(-EINVAL);
    return;
  }

  auto encoder = REFLECTOR(Encoder)::Create<VideoEncoder>(
      ccodec_name, enc_param_str.c_str());
  if (!encoder) {
    LOG("Fail to create video encoder %s<%s>\n", ccodec_name,
        enc_param_str.c_str());
    SetError(-EINVAL);
    return;
  }

  if (!encoder->InitConfig(mc)) {
    LOG("Fail to init config, %s\n", ccodec_name);
    SetError(-EINVAL);
    return;
  }

  void *extra_data = nullptr;
  size_t extra_data_size = 0;
  encoder->GetExtraData(&extra_data, &extra_data_size);
  // TODO: if not h264
  const std::string &output_dt = enc_params[KEY_OUTPUTDATATYPE];
  if (extra_data && extra_data_size > 0 &&
      (output_dt == VIDEO_H264 || output_dt == VIDEO_H265))
    extra_buffer_list = split_h264_separate((const uint8_t *)extra_data,
                                            extra_data_size, gettimeofday());

  enc = encoder;

  SlotMap sm;
  sm.input_slots.push_back(0);
  sm.output_slots.push_back(0);
  if (params[KEY_NEED_EXTRA_OUTPUT] == "y") {
    extra_output = true;
    sm.output_slots.push_back(1);
  }
  sm.process = encode;
  sm.thread_model = Model::ASYNCCOMMON;
  sm.mode_when_full = InputMode::DROPFRONT;
  sm.input_maxcachenum.push_back(3);
  if (!InstallSlotMap(sm, codec_name, 40)) {
    LOG("Fail to InstallSlotMap, %s\n", ccodec_name);
    SetError(-EINVAL);
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
}

DEFINE_FLOW_FACTORY(VideoEncoderFlow, Flow)
// type depends on encoder
const char *FACTORY(VideoEncoderFlow)::ExpectedInputDataType() { return ""; }
const char *FACTORY(VideoEncoderFlow)::OutPutDataType() { return ""; }

} // namespace easymedia
