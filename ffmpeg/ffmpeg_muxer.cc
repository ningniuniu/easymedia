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

#include "muxer.h"

#include <assert.h>

#include "buffer.h"
#include "ffmpeg_utils.h"

namespace easymedia {

class FFMPEGMuxer : public Muxer {
public:
  FFMPEGMuxer(const char *param);
  virtual ~FFMPEGMuxer();
  static const char *GetMuxName() { return "ffmpeg"; }

  virtual bool Init() override;
  virtual bool
  NewMuxerStream(const MediaConfig &mc,
                 const std::shared_ptr<MediaBuffer> &enc_extra_data,
                 int &stream_no) override;
  virtual bool SetIoStream(std::shared_ptr<Stream> output _UNUSED) {
    // do not support
    return false;
  }
  virtual std::shared_ptr<MediaBuffer> WriteHeader(int stream_no);
  virtual std::shared_ptr<MediaBuffer>
  Write(std::shared_ptr<MediaBuffer> orig_data, int stream_no) override;

private:
  std::string path;
  AVFormatContext *context;
  std::vector<AVStream *> streams;
  int nb_streams;
  std::vector<int64_t> first_timestamp;
  std::vector<int64_t> pre_pts;

  class FFMPEG_AV_INIT {
  public:
    FFMPEG_AV_INIT() { avformat_network_init(); }
    ~FFMPEG_AV_INIT() { avformat_network_deinit(); }
  };
  static const FFMPEG_AV_INIT gAVInit;
  static std::shared_ptr<MediaBuffer> empty;
};

std::shared_ptr<MediaBuffer> FFMPEGMuxer::empty =
    std::make_shared<MediaBuffer>();

FFMPEGMuxer::FFMPEGMuxer(const char *param)
    : Muxer(param), context(NULL), nb_streams(0) {
  std::map<std::string, std::string> params;
  std::list<std::pair<const std::string, std::string &>> req_list;
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_PATH, path));
  parse_media_param_match(param, params, req_list);
}

FFMPEGMuxer::~FFMPEGMuxer() {
  if (!context)
    return;
  if (context->pb)
    avio_closep(&context->pb);
  avformat_free_context(context);
}

bool FFMPEGMuxer::Init() {
  if (!empty)
    return false;
  if (path.empty()) {
    LOG("missing path\n");
    return false;
  }
  AVFormatContext *c = NULL;
  auto ret = avformat_alloc_output_context2(&c, NULL, NULL, path.c_str());
  if (!c) {
    fprintf(stderr,
            "avformat_alloc_output_context2 failed for url %s, ret: %d\n",
            path.c_str(), ret);
    return false;
  }
  context = c;
  return true;
}

static bool _convert_to_avcodecparam(AVFormatContext *c, const MediaConfig *mc,
                                     AVCodecParameters *par,
                                     AVRational *time_base) {
  Type type = mc->type;
  switch (type) {
  case Type::Image:
  case Type::Video: {
    const VideoConfig &vc = mc->vid_cfg;
    const ImageConfig &ic = (type == Type::Image) ? mc->img_cfg : vc.image_cfg;
    par->codec_type = AVMEDIA_TYPE_VIDEO;
    par->codec_id = (type == Type::Image)
                        ? AV_CODEC_ID_RAWVIDEO
                        : PixFmtToAVCodecID(ic.image_info.pix_fmt);
    if (par->codec_id == AV_CODEC_ID_NONE)
      return false;
    auto av_fmt = PixFmtToAVPixFmt(ic.image_info.pix_fmt);
    par->codec_tag =
        (type == Type::Image)
            ? avcodec_pix_fmt_to_codec_tag(av_fmt)
            : av_codec_get_tag(c->oformat->codec_tag, par->codec_id);
    par->format = av_fmt;
    par->sw_format = AV_PIX_FMT_NONE; // TODO
    par->width = ic.image_info.width;
    par->height = ic.image_info.height;
    par->video_delay = 0; // TODO

    if (type == Type::Video) {
      par->bit_rate = vc.bit_rate;
      par->profile = vc.profile;
      par->level = vc.level;
      *time_base = (AVRational){1, vc.frame_rate};
    }
  } break;
  case Type::Audio: {
    const AudioConfig &ac = mc->aud_cfg;
    par->codec_type = AVMEDIA_TYPE_AUDIO;
    par->codec_id = SampleFmtToAVCodecID(ac.sample_info.fmt);
    if (par->codec_id == AV_CODEC_ID_NONE)
      return false;
    par->codec_tag = av_codec_get_tag(c->oformat->codec_tag, par->codec_id);
    par->format = SampleFmtToAVSamFmt(ac.sample_info.fmt);
    par->channels = ac.sample_info.channels;
    par->channel_layout = av_get_default_channel_layout(par->channels);
    par->sample_rate = ac.sample_info.sample_rate;
    // par->block_align
    if (par->codec_id == AV_CODEC_ID_MP2)
      par->frame_size = ac.sample_info.nb_samples;
    *time_base = (AVRational){1, par->sample_rate};
  } break;
  case Type::Text: {
    const ImageConfig &ic = mc->img_cfg;
    par->codec_type = AVMEDIA_TYPE_SUBTITLE;
    par->width = ic.image_info.width;
    par->height = ic.image_info.height;
  } break;
  default:
    return false;
  }
  return true;
}

bool FFMPEGMuxer::NewMuxerStream(
    const MediaConfig &mc, const std::shared_ptr<MediaBuffer> &enc_extra_data,
    int &stream_no) {
  // if (oc->oformat->flags & AVFMT_GLOBALHEADER)
  //   c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  stream_no = -1;
  AVRational time_base = (AVRational){1, 1};
  AVCodecParameters *codecpar = avcodec_parameters_alloc();
  if (!codecpar) {
    LOG_NO_MEMORY();
    return false;
  }
  if (!_convert_to_avcodecparam(context, &mc, codecpar, &time_base))
    return false;
  AVStream *s = avformat_new_stream(context, NULL);
  if (!s) {
    LOG("avformat_new_stream failed\n");
    return false;
  }
  assert(s->index < 64);
  stream_no = s->index;
  LOGD("new stream index %d\n", stream_no);
  s->id = context->nb_streams - 1;
  *(s->codecpar) = *codecpar;
  s->time_base = time_base;
#if 1 // make av_dump_format correct
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  if (mc.type == Type::Video) {
    const VideoConfig &vc = mc.vid_cfg;
    const auto &info = vc.image_cfg.image_info;
    s->codec->qmin = vc.qp_min;
    s->codec->qmax = vc.qp_max;
    s->codec->coded_width = info.vir_width;
    s->codec->coded_height = info.vir_height;
  }
#pragma GCC diagnostic pop
#endif
  avcodec_parameters_free(&codecpar);
  if (enc_extra_data && enc_extra_data->GetValidSize() > 0) {
    auto size = enc_extra_data->GetValidSize();
    s->codecpar->extradata =
        (uint8_t *)av_mallocz(size + AV_INPUT_BUFFER_PADDING_SIZE);
    if (!s->codecpar->extradata) {
      LOG_NO_MEMORY();
      return false;
    }
    memcpy(s->codecpar->extradata, enc_extra_data->GetPtr(), size);
    s->codecpar->extradata_size = size;
  }
  if ((int)streams.size() <= stream_no) {
    streams.resize(stream_no + 1);
    streams[stream_no] = NULL;
    first_timestamp.resize(stream_no + 1, -1);
    pre_pts.resize(stream_no + 1, 0);
  }
  assert(!streams[stream_no]);
  streams[stream_no] = s;
  nb_streams++;
  assert((int)context->nb_streams == nb_streams);

  return true;
}

std::shared_ptr<MediaBuffer> FFMPEGMuxer::WriteHeader(int stream_no) {
  if (stream_no < 0 || stream_no >= (int)streams.size()) {
    LOG("Invalid stream no : %d\n", stream_no);
    return nullptr;
  }
  const char *url = path.c_str();
  if (context->pb)
    return empty;
#ifndef NDEBUG
  av_dump_format(context, stream_no, url, 1);
#endif
  int ret;
  if (!(context->oformat->flags & AVFMT_NOFILE)) {
    ret = avio_open(&context->pb, url, AVIO_FLAG_WRITE);
    if (ret < 0) {
      PrintAVError(ret, "Could not open", path.c_str());
      return nullptr;
    }
  } else {
    if (url && !(context->url = av_strdup(url)))
      return nullptr;
  }
  ret = avformat_write_header(context, NULL);
  if (ret < 0) {
    PrintAVError(ret, "Fail to write header", path.c_str());
    return nullptr;
  }
#ifndef NDEBUG
  int i = 0;
  for (auto s : streams) {
    LOGD("stream index %d, after write header time_base: %d/%d\n", i++,
         s->time_base.num, s->time_base.den);
  }
#endif
  return empty;
}

std::shared_ptr<MediaBuffer>
FFMPEGMuxer::Write(std::shared_ptr<MediaBuffer> data, int stream_no) {
  assert(stream_no >= 0 && stream_no < (int)streams.size());
  int ret;
  bool eof = data->IsEOF();
  size_t size = data->GetValidSize();
  if (size > 0) {
    auto s = streams[stream_no];
    AVPacket avpkt;
    av_init_packet(&avpkt);
    avpkt.data = (uint8_t *)data->GetPtr();
    avpkt.size = size;
    avpkt.stream_index = s->index;
    if (data->GetUserFlag() & MediaBuffer::kIntra)
      avpkt.flags |= AV_PKT_FLAG_KEY;
    // if (data->GetUserFlag() & MediaBuffer::kSingleNalUnit)
    //   avpkt.flags |= AV_PKT_FLAG_ONE_NAL;
    int64_t diff = 0;
    int64_t pts = 0;
    if (first_timestamp[stream_no] < 0) {
      first_timestamp[stream_no] = data->GetUSTimeStamp();
    } else {
      diff = data->GetUSTimeStamp() - first_timestamp[stream_no];
      // (enum AVRounding)(AV_ROUND_UP | AV_ROUND_PASS_MINMAX)
      pts = av_rescale_rnd(diff, s->time_base.den, s->time_base.num * 1000000LL,
                           AV_ROUND_UP);
      if (pts <= pre_pts[stream_no])
        pts = pre_pts[stream_no] + 1;
      pre_pts[stream_no] = pts;
    }
    avpkt.dts = avpkt.pts = pts;
    LOGD("[%d] pts = %ld, num/den =%d/%d\n", stream_no, pts, s->time_base.num,
         s->time_base.den);
    ret = av_write_frame(context, &avpkt);
    av_packet_unref(&avpkt);
    if (ret < 0) {
      PrintAVError(ret, "Fail to write frame", path.c_str());
      if (!eof)
        return nullptr;
    }
  }

  if (eof) {
    ret = av_write_trailer(context);
    if (ret < 0) {
      PrintAVError(ret, "Fail to write trailer", path.c_str());
      return nullptr;
    }
  }

  return empty;
}

DEFINE_COMMON_MUXER_FACTORY(FFMPEGMuxer)
const char *FACTORY(FFMPEGMuxer)::ExpectedInputDataType() {
  return TYPE_ANYTHING;
}
const char *FACTORY(FFMPEGMuxer)::OutPutDataType() { return TYPE_NOTHING; }

} // namespace easymedia
