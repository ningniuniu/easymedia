/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <mpp/mpp_log.h>

#include "buffer.h"

#include "mpp_encoder.h"

namespace rkmedia {

class MPPH264Encoder : public MPPEncoder {
public:
  static const int kMPPH264MinBps = 2 * 1000;
  static const int kMPPH264MaxBps = 98 * 1000 * 1000;

  MPPH264Encoder(const char *param _UNUSED) {
    SetMppCodeingType(MPP_VIDEO_CodingAVC);
  }
  virtual ~MPPH264Encoder() = default;

  static const char *GetCodecName() { return "rkmpp_h264"; }

  virtual bool InitConfig(const MediaConfig &cfg) override;

  virtual int Process(std::shared_ptr<MediaBuffer> input,
                      std::shared_ptr<MediaBuffer> output,
                      std::shared_ptr<MediaBuffer> extra_output) override;

protected:
  // Change configs which are not contained in sps/pps.
  virtual bool CheckConfigChange(
      std::pair<uint32_t, std::shared_ptr<ParameterBuffer>>) override;
};

static int CalcMppBps(MppEncRcCfg *rc_cfg, int bps) {
  if (bps > MPPH264Encoder::kMPPH264MaxBps) {
    LOG("bps <%d> too large\n", bps);
    return -1;
  }

  switch (rc_cfg->rc_mode) {
  case MPP_ENC_RC_MODE_CBR:
    // constant bitrate has very small bps range of 1/16 bps
    rc_cfg->bps_target = bps;
    rc_cfg->bps_max = bps * 17 / 16;
    rc_cfg->bps_min = bps * 15 / 16;
    break;
  case MPP_ENC_RC_MODE_VBR:
    // variable bitrate has large bps range
    rc_cfg->bps_target = bps;
    rc_cfg->bps_max = bps * 3 / 2;
    rc_cfg->bps_min = bps * 1 / 2;
    break;
  default:
    // TODO
    LOG("right now rc_mode=%d is untested\n", rc_cfg->rc_mode);
    return -1;
  }

  if (rc_cfg->bps_max > MPPH264Encoder::kMPPH264MaxBps)
    rc_cfg->bps_max = MPPH264Encoder::kMPPH264MaxBps;
  if (rc_cfg->bps_min < MPPH264Encoder::kMPPH264MinBps)
    rc_cfg->bps_min = MPPH264Encoder::kMPPH264MinBps;

  return 0;
}

static MppEncRcMode get_rc_mode(const char *rc_mode) {
  if (!rc_mode)
    return MPP_ENC_RC_MODE_BUTT;

  if (!strcasecmp("vbr", rc_mode))
    return MPP_ENC_RC_MODE_VBR;
  else if (!strcasecmp("cbr", rc_mode))
    return MPP_ENC_RC_MODE_CBR;

  return MPP_ENC_RC_MODE_BUTT;
}

static MppEncRcQuality get_rc_quality(const char *quality) {
  if (!quality)
    return MPP_ENC_RC_QUALITY_BUTT;

  if (!strcasecmp("worst", quality))
    return MPP_ENC_RC_QUALITY_WORST;
  else if (!strcasecmp("worse", quality))
    return MPP_ENC_RC_QUALITY_WORSE;
  else if (!strcasecmp("medium", quality))
    return MPP_ENC_RC_QUALITY_MEDIUM;
  else if (!strcasecmp("better", quality))
    return MPP_ENC_RC_QUALITY_BETTER;
  else if (!strcasecmp("best", quality))
    return MPP_ENC_RC_QUALITY_BEST;
  else if (!strcasecmp("cqp", quality))
    return MPP_ENC_RC_QUALITY_CQP;
  else if (!strcasecmp("aq_only", quality))
    return MPP_ENC_RC_QUALITY_AQ_ONLY;

  return MPP_ENC_RC_QUALITY_BUTT;
}

bool MPPH264Encoder::InitConfig(const MediaConfig &cfg) {
  const VideoConfig &vconfig = cfg.vid_cfg;
  const ImageConfig &img_cfg = vconfig.image_cfg;
  MppFrameFormat pic_type;
  MpiCmd mpi_cmd = MPP_CMD_BASE;
  MppParam param = NULL;
  MppEncCfgSet mpp_cfg;
  MppEncRcCfg *rc_cfg = &mpp_cfg.rc;
  MppEncPrepCfg *prep_cfg = &mpp_cfg.prep;
  MppEncCodecCfg *codec_cfg = &mpp_cfg.codec;
  unsigned int need_block = 1;
  int dummy = 1;
  int profile = 100;
  int fps = vconfig.frame_rate;
  int bps = vconfig.bit_rate;
  int gop = vconfig.gop_size;

  if (fps < 1)
    fps = 1;
  else if (fps > ((1 << 16) - 1))
    fps = (1 << 16) - 1;

  mpi_cmd = MPP_SET_INPUT_BLOCK;
  param = &need_block;
  int ret = EncodeControl(mpi_cmd, param);
  if (ret != 0) {
    LOG("mpp control set input block failed ret %d\n", ret);
    return false;
  }

  pic_type = ConvertToMppPixFmt(img_cfg.image_info.pix_fmt);
  if (pic_type == -1) {
    LOG("error input pixel format\n");
    return false;
  }
  // NOTE: in mpp, chroma_addr = luma_addr + hor_stride*ver_stride,
  // so if there is no gap between luma and chroma in yuv,
  // hor_stride MUST be set to be width, ver_stride MUST be set to height.
  prep_cfg->change =
      MPP_ENC_PREP_CFG_CHANGE_INPUT | MPP_ENC_PREP_CFG_CHANGE_FORMAT;
  prep_cfg->width = img_cfg.image_info.width;
  prep_cfg->height = img_cfg.image_info.height;
  if (pic_type == MPP_FMT_YUV422_YUYV || pic_type == MPP_FMT_YUV422_UYVY)
    prep_cfg->hor_stride = img_cfg.image_info.vir_width * 2;
  else
    prep_cfg->hor_stride = img_cfg.image_info.vir_width;
  prep_cfg->ver_stride = img_cfg.image_info.vir_height;
  prep_cfg->format = pic_type;

  LOG("encode w x h(%d[%d] x %d[%d])\n", prep_cfg->width, prep_cfg->hor_stride,
      prep_cfg->height, prep_cfg->ver_stride);
  ret = EncodeControl(MPP_ENC_SET_PREP_CFG, prep_cfg);
  if (ret) {
    LOG("mpi control enc set prep cfg failed ret %d\n", ret);
    return false;
  }

  param = &dummy;
  ret = EncodeControl(MPP_ENC_PRE_ALLOC_BUFF, param);
  if (ret) {
    LOG("mpi control pre alloc buff failed ret %d\n", ret);
    return false;
  }

  rc_cfg->change = MPP_ENC_RC_CFG_CHANGE_ALL;

  rc_cfg->rc_mode = get_rc_mode(vconfig.rc_mode);
  if (rc_cfg->rc_mode == MPP_ENC_RC_MODE_BUTT) {
    LOG("Invalid rc mode %s\n", vconfig.rc_mode);
    return false;
  }
  rc_cfg->quality = get_rc_quality(vconfig.rc_quality);
  if (rc_cfg->quality == MPP_ENC_RC_QUALITY_BUTT) {
    LOG("Invalid rc quality %s\n", vconfig.rc_quality);
    return false;
  }

  CalcMppBps(rc_cfg, bps);

  // fix input / output frame rate
  rc_cfg->fps_in_flex = 0;
  rc_cfg->fps_in_num = fps;
  rc_cfg->fps_in_denorm = 1;
  rc_cfg->fps_out_flex = 0;
  rc_cfg->fps_out_num = fps;
  rc_cfg->fps_out_denorm = 1;

  rc_cfg->gop = gop;
  rc_cfg->skip_cnt = 0;

  LOG("encode bps %d fps %d gop %d\n", bps, fps, gop);
  ret = EncodeControl(MPP_ENC_SET_RC_CFG, rc_cfg);
  if (ret) {
    LOG("mpi control enc set rc cfg failed ret %d\n", ret);
    return false;
  }

  profile = vconfig.profile;
  if (profile != 66 && profile != 77)
    profile = 100; // default PROFILE_HIGH 100

  codec_cfg->coding = MPP_VIDEO_CodingAVC;
  codec_cfg->h264.change =
      MPP_ENC_H264_CFG_CHANGE_PROFILE | MPP_ENC_H264_CFG_CHANGE_ENTROPY;
  codec_cfg->h264.profile = profile;
  codec_cfg->h264.level = vconfig.level;
  codec_cfg->h264.entropy_coding_mode =
      (profile == 66 || profile == 77) ? (0) : (1);
  codec_cfg->h264.cabac_init_idc = 0;

  // setup QP on CQP mode
  codec_cfg->h264.change |= MPP_ENC_H264_CFG_CHANGE_QP_LIMIT;
  codec_cfg->h264.qp_init = img_cfg.qp_init;
  codec_cfg->h264.qp_min = vconfig.qp_min;
  codec_cfg->h264.qp_max = vconfig.qp_max;
  codec_cfg->h264.qp_max_step = vconfig.qp_step;

  LOG("encode profile %d level %d init_qp %d\n", profile, codec_cfg->h264.level,
      img_cfg.qp_init);
  ret = EncodeControl(MPP_ENC_SET_CODEC_CFG, codec_cfg);
  if (ret) {
    LOG("mpi control enc set codec cfg failed ret %d\n", ret);
    return false;
  }

  if (bps >= 50000000) {
    RK_U32 qp_scale = 2; // 1 or 2
    ret = EncodeControl(MPP_ENC_SET_QP_RANGE, &qp_scale);
    if (ret) {
      LOG("mpi control enc set qp scale failed ret %d\n", ret);
      return false;
    }
    LOG("qp_scale:%d\n", qp_scale);
  }

  MppPacket packet = nullptr;
  ret = EncodeControl(MPP_ENC_GET_EXTRA_INFO, &packet);
  if (ret) {
    LOG("mpi control enc get extra info failed\n");
    return false;
  }

  // Get and write sps/pps for H.264
  if (packet) {
    void *ptr = mpp_packet_get_pos(packet);
    size_t len = mpp_packet_get_length(packet);
    if (!SetExtraData(ptr, len)) {
      LOG("SetExtraData failed\n");
      return false;
    }
    packet = NULL;
  }

#if MPP_SUPPORT_HW_OSD
  MppEncOSDPlt palette;
  memcpy((uint8_t *)palette.buf, (uint8_t *)yuv444_palette_table,
         PALETTE_TABLE_LEN * 4);

  ret = EncodeControl(MPP_ENC_SET_OSD_PLT_CFG, &palette);
  if (ret != 0) {
    LOG("encode_control set osd plt cfg failed\n");
    return false;
  }
#endif

  return MPPEncoder::InitConfig(cfg);
}

bool MPPH264Encoder::CheckConfigChange(
    std::pair<uint32_t, std::shared_ptr<ParameterBuffer>> change_pair) {
  VideoConfig &vconfig = GetConfig().vid_cfg;
  MppEncRcCfg rc_cfg;

  uint32_t change = change_pair.first;
  std::shared_ptr<ParameterBuffer> &val = change_pair.second;

  if (change & kFrameRateChange) {
    int new_frame_rate = val->GetValue();
    mpp_assert(new_frame_rate > 0 && new_frame_rate < 120);
    rc_cfg.change = MPP_ENC_RC_CFG_CHANGE_FPS_OUT;
    rc_cfg.fps_out_flex = 0;
    rc_cfg.fps_out_num = new_frame_rate;
    rc_cfg.fps_out_denorm = 1;
    if (EncodeControl(MPP_ENC_SET_RC_CFG, &rc_cfg) != 0) {
      LOG("encode_control MPP_ENC_SET_RC_CFG(MPP_ENC_RC_CFG_CHANGE_FPS_OUT) "
          "error!\n");
      return false;
    }
    vconfig.frame_rate = new_frame_rate;
  } else if (change & kBitRateChange) {
    int new_bit_rate = val->GetValue();
    mpp_assert(new_bit_rate > 0 && new_bit_rate < 60 * 1000 * 1000);
    rc_cfg.change = MPP_ENC_RC_CFG_CHANGE_BPS;
    rc_cfg.rc_mode = get_rc_mode(vconfig.rc_mode);
    rc_cfg.quality = get_rc_quality(vconfig.rc_quality);
    CalcMppBps(&rc_cfg, new_bit_rate);
    if (EncodeControl(MPP_ENC_SET_RC_CFG, &rc_cfg) != 0) {
      LOG("encode_control MPP_ENC_SET_RC_CFG(MPP_ENC_RC_CFG_CHANGE_BPS) "
          "error!\n");
      return false;
    }
    vconfig.bit_rate = new_bit_rate;
    return true;
  } else if (change & kForceIdrFrame) {
    if (EncodeControl(MPP_ENC_SET_IDR_FRAME, nullptr) != 0) {
      LOG("encode_control MPP_ENC_SET_IDR_FRAME error!\n");
      return false;
    }
  } else if (change & kQPChange) {
    MppEncCodecCfg codec_cfg;
    mpp_assert(val->GetSize() == sizeof(int) * 4);
    int *values = (int *)val->GetBufferPtr();
    codec_cfg.h264.change = MPP_ENC_H264_CFG_CHANGE_QP_LIMIT;
    codec_cfg.h264.qp_init = values[0];
    codec_cfg.h264.qp_min = values[1];
    codec_cfg.h264.qp_max = values[2];
    codec_cfg.h264.qp_max_step = values[3];
    if (EncodeControl(MPP_ENC_SET_CODEC_CFG, &codec_cfg) != 0) {
      LOG("encode_control MPP_ENC_SET_CODEC_CFG error!\n");
      return false;
    }
    vconfig.image_cfg.qp_init = values[0];
    vconfig.qp_min = values[1];
    vconfig.qp_max = values[2];
    vconfig.qp_step = values[3];
  }
#if MPP_SUPPORT_HW_OSD
  else if (change & kOSDDataChange) {
    // type: MppEncOSDData*
    void *param = val->GetUserData();
    if (EncodeControl(MPP_ENC_SET_OSD_DATA_CFG, param) != 0) {
      LOG("encode_control MPP_ENC_SET_OSD_DATA_CFG error!\n");
      return false;
    }
  }
#endif
  else {
    LOG("Unknown request change 0x%08x!\n", change);
    return false;
  }

  return true;
}

int MPPH264Encoder::Process(std::shared_ptr<MediaBuffer> input,
                            std::shared_ptr<MediaBuffer> output,
                            std::shared_ptr<MediaBuffer> extra_output) {
  int ret = MPPEncoder::Process(input, output, extra_output);
  if (!ret)
    output->SetType(MediaBuffer::Type::Video);

  return ret;
}

DEFINE_MPP_ENCODER_FACTORY(MPPH264Encoder)

} // namespace rkmedia
