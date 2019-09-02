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
#include "encoder.h"
#include "ffmpeg_utils.h"
#include "media_type.h"

namespace easymedia {

// A encoder which call the ffmpeg interface.
class FFMPEGAudioEncoder : public AudioEncoder {
public:
  FFMPEGAudioEncoder(const char *param);
  virtual ~FFMPEGAudioEncoder();
  static const char *GetCodecName() { return "ffmpeg_aud"; }
  virtual bool Init() override;
  virtual bool InitConfig(const MediaConfig &cfg) override;
  virtual int Process(const std::shared_ptr<MediaBuffer> &,
                      std::shared_ptr<MediaBuffer> &,
                      std::shared_ptr<MediaBuffer>) override {
    errno = ENOSYS;
    return -1;
  }
  // avcodec_send_frame()/avcodec_receive_packet()
  virtual int SendInput(const std::shared_ptr<MediaBuffer> &input) override;
  virtual std::shared_ptr<MediaBuffer> FetchOutput() override;

private:
  AVCodec *av_codec;
  AVCodecContext *avctx;
  enum AVSampleFormat input_fmt;
  std::string output_data_type;
  std::string ff_codec_name;
};

FFMPEGAudioEncoder::FFMPEGAudioEncoder(const char *param)
    : av_codec(nullptr), avctx(nullptr), input_fmt(AV_SAMPLE_FMT_NONE) {
  std::map<std::string, std::string> params;
  std::list<std::pair<const std::string, std::string &>> req_list;
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_OUTPUTDATATYPE, output_data_type));
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_NAME, ff_codec_name));
  parse_media_param_match(param, params, req_list);
}

FFMPEGAudioEncoder::~FFMPEGAudioEncoder() {
  if (avctx)
    avcodec_free_context(&avctx);
}

bool FFMPEGAudioEncoder::Init() {
  if (output_data_type.empty()) {
    LOG("missing %s\n", KEY_OUTPUTDATATYPE);
    return false;
  }
  output_fmt = StringToSampleFmt(output_data_type.c_str());
  if (!ff_codec_name.empty()) {
    av_codec = avcodec_find_encoder_by_name(ff_codec_name.c_str());
  } else {
    AVCodecID id = SampleFmtToAVCodecID(output_fmt);
    av_codec = avcodec_find_encoder(id);
  }
  if (!av_codec) {
    LOG("Fail to find ffmpeg codec, request codec name=%s, or format=%s\n",
        ff_codec_name.c_str(), output_data_type.c_str());
    return false;
  }
  avctx = avcodec_alloc_context3(av_codec);
  if (!avctx) {
    LOG("Fail to avcodec_alloc_context3\n");
    return false;
  }
  LOG("av codec name=%s\n",
      av_codec->long_name ? av_codec->long_name : av_codec->name);
  return true;
}

static bool check_sample_fmt(const AVCodec *codec,
                             enum AVSampleFormat sample_fmt) {
  const enum AVSampleFormat *p = codec->sample_fmts;
  if (!p)
    return true;
  while (*p != AV_SAMPLE_FMT_NONE) {
    if (*p++ == sample_fmt)
      return true;
  }
  LOG("av codec_id [0x%08x] does not support av sample fmt [%d]\n", codec->id,
      sample_fmt);
  return false;
}

static bool check_sample_rate(const AVCodec *codec, int sample_rate) {
  const int *p = codec->supported_samplerates;
  if (!p)
    return true;
  while (*p != 0) {
    if (*p++ == sample_rate)
      return true;
  }
  LOG("av codec_id [0x%08x] does not support sample_rate [%d]\n", codec->id,
      sample_rate);
  return false;
}

static bool check_channel_layout(const AVCodec *codec,
                                 uint64_t channel_layout) {
  const uint64_t *p = codec->channel_layouts;
  if (!p)
    return true;
  while (*p != 0) {
    if (*p++ == channel_layout)
      return true;
  }
  LOG("av codec_id [0x%08x] does not support audio channel_layout [%d]\n",
      codec->id, (int)channel_layout);
  return false;
}

bool FFMPEGAudioEncoder::InitConfig(const MediaConfig &cfg) {
  const AudioConfig &ac = cfg.aud_cfg;
  input_fmt = avctx->sample_fmt = SampleFmtToAVSamFmt(ac.sample_info.fmt);
  if (!check_sample_fmt(av_codec, avctx->sample_fmt))
    return false;
  avctx->bit_rate = ac.bit_rate;
  avctx->sample_rate = ac.sample_info.sample_rate;
  if (!check_sample_rate(av_codec, avctx->sample_rate))
    return false;
  avctx->channels = ac.sample_info.channels;
  avctx->channel_layout = av_get_default_channel_layout(avctx->channels);
  if (!check_channel_layout(av_codec, avctx->channel_layout))
    return false;
  // avctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  int av_ret = avcodec_open2(avctx, av_codec, NULL);
  if (av_ret < 0) {
    PrintAVError(av_ret, "Fail to avcodec_open2", av_codec->long_name);
    return false;
  }
  auto mc = cfg;
  mc.type = Type::Audio;
  if (!(avctx->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE))
    mc.aud_cfg.sample_info.nb_samples = avctx->frame_size;

  return AudioEncoder::InitConfig(mc);
}

int FFMPEGAudioEncoder::SendInput(const std::shared_ptr<MediaBuffer> &input) {
  AVFrame *f = NULL;
  AVFrame frame;
  if (input->IsValid()) {
    assert(input && input->GetType() == Type::Audio);
    auto in = std::static_pointer_cast<SampleBuffer>(input);
    memset(&frame, 0, sizeof(frame));
    auto &info = in->GetSampleInfo();
    if (in->GetSamples() > 0) {
      frame.channels = info.channels;
      frame.format = input_fmt;
      assert(input_fmt == SampleFmtToAVSamFmt(in->GetSampleFormat()));
      frame.nb_samples = in->GetSamples();
      frame.channel_layout = av_get_default_channel_layout(info.channels);
      frame.data[0] = (uint8_t *)in->GetPtr();
      frame.extended_data = frame.data;
      frame.pts = in->GetUSTimeStamp();
      f = &frame;
    }
  }
  int ret = avcodec_send_frame(avctx, f);
  if (ret < 0) {
    if (ret == AVERROR(EAGAIN))
      return -EAGAIN;
    PrintAVError(ret, "Fail to send frame to encoder", av_codec->long_name);
    return -1;
  }
  return 0;
}

static int __ffmpeg_packet_free(void *arg) {
  auto pkt = (AVPacket *)arg;
  av_packet_free(&pkt);
  return 0;
}

std::shared_ptr<MediaBuffer> FFMPEGAudioEncoder::FetchOutput() {
  auto pkt = av_packet_alloc();
  if (!pkt)
    return nullptr;
  MediaBuffer mb(pkt->data, 0, -1, pkt, __ffmpeg_packet_free);
  auto buffer = std::make_shared<SampleBuffer>(mb, output_fmt);
  if (!buffer) {
    errno = ENOMEM;
    return nullptr;
  }
  int ret = avcodec_receive_packet(avctx, pkt);
  if (ret < 0) {
    if (ret == AVERROR(EAGAIN)) {
      errno = EAGAIN;
      return nullptr;
    } else if (ret == AVERROR_EOF) {
      buffer->SetEOF(true);
      return buffer;
    }
    errno = EFAULT;
    PrintAVError(ret, "Fail to receiver from encoder", av_codec->long_name);
    return nullptr;
  }
  buffer->SetPtr(pkt->data);
  buffer->SetValidSize(pkt->size);
  buffer->SetUSTimeStamp(pkt->pts);
  return buffer;
}

DEFINE_AUDIO_ENCODER_FACTORY(FFMPEGAudioEncoder)
const char *FACTORY(FFMPEGAudioEncoder)::ExpectedInputDataType() {
  return AUDIO_PCM;
}
const char *FACTORY(FFMPEGAudioEncoder)::OutPutDataType() {
  return TYPE_ANYTHING;
}

} // namespace easymedia
