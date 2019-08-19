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

#include "buffer.h"

#include "media_type.h"
#include "mpp_encoder.h"

namespace easymedia {

class MPPConfig {
public:
  virtual ~MPPConfig() = default;
  virtual bool InitConfig(MPPEncoder &mpp_enc, const MediaConfig &cfg) = 0;
  virtual bool CheckConfigChange(MPPEncoder &mpp_enc, uint32_t change,
                                 ParameterBuffer *val) = 0;
};

class MPPMJPEGConfig : public MPPConfig {
public:
  virtual bool InitConfig(MPPEncoder &mpp_enc, const MediaConfig &cfg) override;
  virtual bool CheckConfigChange(MPPEncoder &mpp_enc, uint32_t change,
                                 ParameterBuffer *val) override;
};

static bool MppEncPrepConfig(MppEncPrepCfg &prep_cfg,
                             const ImageInfo &image_info) {
  MppFrameFormat pic_type = ConvertToMppPixFmt(image_info.pix_fmt);
  if (pic_type == -1) {
    LOG("error input pixel format\n");
    return false;
  }
  memset(&prep_cfg, 0, sizeof(prep_cfg));
  prep_cfg.change =
      MPP_ENC_PREP_CFG_CHANGE_INPUT | MPP_ENC_PREP_CFG_CHANGE_FORMAT;
  prep_cfg.width = image_info.width;
  prep_cfg.height = image_info.height;
  if (pic_type == MPP_FMT_YUV422_YUYV || pic_type == MPP_FMT_YUV422_UYVY)
    prep_cfg.hor_stride = image_info.vir_width * 2;
  else
    prep_cfg.hor_stride = image_info.vir_width;
  prep_cfg.ver_stride = image_info.vir_height;
  prep_cfg.format = pic_type;
  LOG("encode w x h(%d[%d] x %d[%d])\n", prep_cfg.width, prep_cfg.hor_stride,
      prep_cfg.height, prep_cfg.ver_stride);
  return true;
}

bool MPPMJPEGConfig::InitConfig(MPPEncoder &mpp_enc, const MediaConfig &cfg) {
  const ImageConfig &img_cfg = cfg.img_cfg;
  MppEncPrepCfg prep_cfg;

  if (!MppEncPrepConfig(prep_cfg, img_cfg.image_info))
    return false;
  int ret = mpp_enc.EncodeControl(MPP_ENC_SET_PREP_CFG, &prep_cfg);
  if (ret) {
    LOG("mpi control enc set cfg failed\n");
    return false;
  }
  mpp_enc.GetConfig().img_cfg.image_info = img_cfg.image_info;
  mpp_enc.GetConfig().type = Type::Image;
  ParameterBuffer change;
  change.SetValue(img_cfg.qp_init);
  return CheckConfigChange(mpp_enc, VideoEncoder::kQPChange, &change);
}

bool MPPMJPEGConfig::CheckConfigChange(MPPEncoder &mpp_enc, uint32_t change,
                                       ParameterBuffer *val) {
  ImageConfig &iconfig = mpp_enc.GetConfig().img_cfg;
  if (change & VideoEncoder::kQPChange) {
    int quant = val->GetValue();
    MppEncCodecCfg codec_cfg;
    memset(&codec_cfg, 0, sizeof(codec_cfg));
    codec_cfg.coding = MPP_VIDEO_CodingMJPEG;
    codec_cfg.jpeg.change = MPP_ENC_JPEG_CFG_CHANGE_QP;
    quant = std::max(1, std::min(quant, 10));
    codec_cfg.jpeg.quant = quant;
    int ret = mpp_enc.EncodeControl(MPP_ENC_SET_CODEC_CFG, &codec_cfg);
    if (ret) {
      LOG("mpi control enc change codec cfg failed, ret=%d\n", ret);
      return false;
    }
    LOG("mjpeg quant = %d\n", quant);
    iconfig.qp_init = quant;
  } else {
    LOG("Unsupport request change 0x%08x!\n", change);
    return false;
  }

  return true;
}

class MPPCommonConfig : public MPPConfig {
public:
  static const int kMPPMinBps = 2 * 1000;
  static const int kMPPMaxBps = 98 * 1000 * 1000;

  MPPCommonConfig(MppCodingType type) : code_type(type) {}
  virtual bool InitConfig(MPPEncoder &mpp_enc, const MediaConfig &cfg) override;
  virtual bool CheckConfigChange(MPPEncoder &mpp_enc, uint32_t change,
                                 ParameterBuffer *val) override;

private:
  MppCodingType code_type;
};

static int CalcMppBps(MppEncRcCfg *rc_cfg, int bps) {
  if (bps > MPPCommonConfig::kMPPMaxBps) {
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

  if (rc_cfg->bps_max > MPPCommonConfig::kMPPMaxBps)
    rc_cfg->bps_max = MPPCommonConfig::kMPPMaxBps;
  if (rc_cfg->bps_min < MPPCommonConfig::kMPPMinBps)
    rc_cfg->bps_min = MPPCommonConfig::kMPPMinBps;

  return 0;
}

bool MPPCommonConfig::InitConfig(MPPEncoder &mpp_enc, const MediaConfig &cfg) {
  VideoConfig vconfig = cfg.vid_cfg;
  const ImageConfig &img_cfg = vconfig.image_cfg;
  MpiCmd mpi_cmd = MPP_CMD_BASE;
  MppParam param = NULL;
  MppEncCfgSet mpp_cfg;
  MppEncRcCfg &rc_cfg = mpp_cfg.rc;
  MppEncPrepCfg &prep_cfg = mpp_cfg.prep;
  MppEncCodecCfg &codec_cfg = mpp_cfg.codec;
  unsigned int need_block = 1;
  int dummy = 1;

  mpi_cmd = MPP_SET_INPUT_BLOCK;
  param = &need_block;
  int ret = mpp_enc.EncodeControl(mpi_cmd, param);
  if (ret != 0) {
    LOG("mpp control set input block failed ret %d\n", ret);
    return false;
  }

  if (!MppEncPrepConfig(prep_cfg, img_cfg.image_info))
    return false;
  ret = mpp_enc.EncodeControl(MPP_ENC_SET_PREP_CFG, &prep_cfg);
  if (ret) {
    LOG("mpi control enc set prep cfg failed ret %d\n", ret);
    return false;
  }

  param = &dummy;
  ret = mpp_enc.EncodeControl(MPP_ENC_PRE_ALLOC_BUFF, param);
  if (ret) {
    LOG("mpi control pre alloc buff failed ret %d\n", ret);
    return false;
  }

  rc_cfg.change = MPP_ENC_RC_CFG_CHANGE_ALL;

  rc_cfg.rc_mode = GetMPPRCMode(vconfig.rc_mode);
  if (rc_cfg.rc_mode == MPP_ENC_RC_MODE_BUTT) {
    LOG("Invalid rc mode %s\n", vconfig.rc_mode);
    return false;
  }
  rc_cfg.quality = GetMPPRCQuality(vconfig.rc_quality);
  if (rc_cfg.quality == MPP_ENC_RC_QUALITY_BUTT) {
    LOG("Invalid rc quality %s\n", vconfig.rc_quality);
    return false;
  }

  int bps = vconfig.bit_rate;
  int fps = std::max(1, std::min(vconfig.frame_rate, (1 << 16) - 1));
  int gop = vconfig.gop_size;

  if (CalcMppBps(&rc_cfg, bps) < 0)
    return false;
  // fix input / output frame rate
  rc_cfg.fps_in_flex = 0;
  rc_cfg.fps_in_num = fps;
  rc_cfg.fps_in_denorm = 1;
  rc_cfg.fps_out_flex = 0;
  rc_cfg.fps_out_num = fps;
  rc_cfg.fps_out_denorm = 1;

  rc_cfg.gop = gop;
  rc_cfg.skip_cnt = 0;

  vconfig.bit_rate = rc_cfg.bps_target;
  vconfig.frame_rate = fps;
  LOG("encode bps %d fps %d gop %d\n", bps, fps, gop);
  ret = mpp_enc.EncodeControl(MPP_ENC_SET_RC_CFG, &rc_cfg);
  if (ret) {
    LOG("mpi control enc set rc cfg failed ret %d\n", ret);
    return false;
  }

  int profile = vconfig.profile;
  if (profile != 66 && profile != 77)
    vconfig.profile = profile = 100; // default PROFILE_HIGH 100

  codec_cfg.coding = code_type;
  switch (code_type) {
  case MPP_VIDEO_CodingAVC:
    codec_cfg.h264.change =
        MPP_ENC_H264_CFG_CHANGE_PROFILE | MPP_ENC_H264_CFG_CHANGE_ENTROPY;
    codec_cfg.h264.profile = profile;
    codec_cfg.h264.level = vconfig.level;
    codec_cfg.h264.entropy_coding_mode =
        (profile == 66 || profile == 77) ? (0) : (1);
    codec_cfg.h264.cabac_init_idc = 0;

    // setup QP on CQP mode
    codec_cfg.h264.change |= MPP_ENC_H264_CFG_CHANGE_QP_LIMIT;
    codec_cfg.h264.qp_init = img_cfg.qp_init;
    codec_cfg.h264.qp_min = vconfig.qp_min;
    codec_cfg.h264.qp_max = vconfig.qp_max;
    codec_cfg.h264.qp_max_step = vconfig.qp_step;
    break;
  case MPP_VIDEO_CodingHEVC:
    // copy from mpi_enc_test.c
    codec_cfg.h265.change = MPP_ENC_H265_CFG_INTRA_QP_CHANGE;
    codec_cfg.h265.intra_qp = img_cfg.qp_init;
    break;
  default:
    // will never go here, avoid gcc warning
    return false;
  }
  LOG("encode profile %d level %d init_qp %d\n", profile, vconfig.level,
      img_cfg.qp_init);
  ret = mpp_enc.EncodeControl(MPP_ENC_SET_CODEC_CFG, &codec_cfg);
  if (ret) {
    LOG("mpi control enc set codec cfg failed ret %d\n", ret);
    return false;
  }

  if (bps >= 50000000) {
    RK_U32 qp_scale = 2; // 1 or 2
    ret = mpp_enc.EncodeControl(MPP_ENC_SET_QP_RANGE, &qp_scale);
    if (ret) {
      LOG("mpi control enc set qp scale failed ret %d\n", ret);
      return false;
    }
    LOG("qp_scale:%d\n", qp_scale);
  }

  MppPacket packet = nullptr;
  ret = mpp_enc.EncodeControl(MPP_ENC_GET_EXTRA_INFO, &packet);
  if (ret) {
    LOG("mpi control enc get extra info failed\n");
    return false;
  }

  // Get and write sps/pps for H.264/5
  if (packet) {
    void *ptr = mpp_packet_get_pos(packet);
    size_t len = mpp_packet_get_length(packet);
    if (!mpp_enc.SetExtraData(ptr, len)) {
      LOG("SetExtraData failed\n");
      return false;
    }
    mpp_enc.GetExtraData()->SetUserFlag(MediaBuffer::kExtraIntra);
    packet = NULL;
  }

#if MPP_SUPPORT_HW_OSD
  MppEncOSDPlt palette;
  memcpy((uint8_t *)palette.buf, (uint8_t *)yuv444_palette_table,
         PALETTE_TABLE_LEN * 4);

  ret = mpp_enc.EncodeControl(MPP_ENC_SET_OSD_PLT_CFG, &palette);
  if (ret) {
    LOG("encode_control set osd plt cfg failed\n");
    return false;
  }
#endif

  mpp_enc.GetConfig().vid_cfg = vconfig;
  mpp_enc.GetConfig().type = Type::Video;
  return true;
}

bool MPPCommonConfig::CheckConfigChange(MPPEncoder &mpp_enc, uint32_t change,
                                        ParameterBuffer *val) {
  VideoConfig &vconfig = mpp_enc.GetConfig().vid_cfg;
  MppEncRcCfg rc_cfg;

  if (change & VideoEncoder::kFrameRateChange) {
    int new_frame_rate = val->GetValue();
    assert(new_frame_rate > 0 && new_frame_rate < 120);
    rc_cfg.change = MPP_ENC_RC_CFG_CHANGE_FPS_OUT;
    rc_cfg.fps_out_flex = 0;
    rc_cfg.fps_out_num = new_frame_rate;
    rc_cfg.fps_out_denorm = 1;
    if (mpp_enc.EncodeControl(MPP_ENC_SET_RC_CFG, &rc_cfg) != 0) {
      LOG("encode_control MPP_ENC_SET_RC_CFG(MPP_ENC_RC_CFG_CHANGE_FPS_OUT) "
          "error!\n");
      return false;
    }
    vconfig.frame_rate = new_frame_rate;
  } else if (change & VideoEncoder::kBitRateChange) {
    int new_bit_rate = val->GetValue();
    assert(new_bit_rate > 0 && new_bit_rate < 60 * 1000 * 1000);
    rc_cfg.change = MPP_ENC_RC_CFG_CHANGE_BPS;
    rc_cfg.rc_mode = GetMPPRCMode(vconfig.rc_mode);
    rc_cfg.quality = GetMPPRCQuality(vconfig.rc_quality);
    if (CalcMppBps(&rc_cfg, new_bit_rate) < 0)
      return false;
    if (mpp_enc.EncodeControl(MPP_ENC_SET_RC_CFG, &rc_cfg) != 0) {
      LOG("encode_control MPP_ENC_SET_RC_CFG(MPP_ENC_RC_CFG_CHANGE_BPS) "
          "error!\n");
      return false;
    }
    vconfig.bit_rate = new_bit_rate;
  } else if (change & VideoEncoder::kForceIdrFrame) {
    if (mpp_enc.EncodeControl(MPP_ENC_SET_IDR_FRAME, nullptr) != 0) {
      LOG("encode_control MPP_ENC_SET_IDR_FRAME error!\n");
      return false;
    }
  } else if (change & VideoEncoder::kQPChange) {
    if (code_type != MPP_VIDEO_CodingAVC) {
      LOG("TODO codeing type: %d\n", code_type);
      return false;
    }
    MppEncCodecCfg codec_cfg;
    assert(val->GetSize() == sizeof(int) * 4);
    int *values = (int *)val->GetPtr();
    codec_cfg.h264.change = MPP_ENC_H264_CFG_CHANGE_QP_LIMIT;
    codec_cfg.h264.qp_init = values[0];
    codec_cfg.h264.qp_min = values[1];
    codec_cfg.h264.qp_max = values[2];
    codec_cfg.h264.qp_max_step = values[3];
    if (mpp_enc.EncodeControl(MPP_ENC_SET_CODEC_CFG, &codec_cfg) != 0) {
      LOG("encode_control MPP_ENC_SET_CODEC_CFG error!\n");
      return false;
    }
    vconfig.image_cfg.qp_init = values[0];
    vconfig.qp_min = values[1];
    vconfig.qp_max = values[2];
    vconfig.qp_step = values[3];
  }
#if MPP_SUPPORT_HW_OSD
  else if (change & VideoEncoder::kOSDDataChange) {
    // type: MppEncOSDData*
    void *param = val->GetPtr();
    if (mpp_enc.EncodeControl(MPP_ENC_SET_OSD_DATA_CFG, param) != 0) {
      LOG("encode_control MPP_ENC_SET_OSD_DATA_CFG error!\n");
      return false;
    }
  }
#endif
  else {
    LOG("Unsupport request change 0x%08x!\n", change);
    return false;
  }

  return true;
}

class MPPFinalEncoder : public MPPEncoder {
public:
  MPPFinalEncoder(const char *param);
  virtual ~MPPFinalEncoder() {
    if (mpp_config)
      delete mpp_config;
  }

  static const char *GetCodecName() { return "rkmpp"; }
  virtual bool InitConfig(const MediaConfig &cfg) override;

protected:
  // Change configs which are not contained in sps/pps.
  virtual bool CheckConfigChange(
      std::pair<uint32_t, std::shared_ptr<ParameterBuffer>>) override;

  MPPConfig *mpp_config;
};

MPPFinalEncoder::MPPFinalEncoder(const char *param) : mpp_config(nullptr) {
  std::string output_data_type =
      get_media_value_by_key(param, KEY_OUTPUTDATATYPE);
  SetMppCodeingType(output_data_type.empty()
                        ? MPP_VIDEO_CodingUnused
                        : GetMPPCodingType(output_data_type));
}

bool MPPFinalEncoder::InitConfig(const MediaConfig &cfg) {
  assert(!mpp_config);
  switch (coding_type) {
  case MPP_VIDEO_CodingMJPEG:
    mpp_config = new MPPMJPEGConfig();
    break;
  case MPP_VIDEO_CodingAVC:
  case MPP_VIDEO_CodingHEVC:
    mpp_config = new MPPCommonConfig(coding_type);
    break;
  default:
    LOG("Unsupport mpp encode type: %d\n", coding_type);
    return false;
  }
  if (!mpp_config) {
    LOG_NO_MEMORY();
    return false;
  }
  return mpp_config->InitConfig(*this, cfg);
}

bool MPPFinalEncoder::CheckConfigChange(
    std::pair<uint32_t, std::shared_ptr<ParameterBuffer>> change_pair) {
  assert(mpp_config);
  if (!mpp_config)
    return false;
  return mpp_config->CheckConfigChange(*this, change_pair.first,
                                       change_pair.second.get());
}

DEFINE_VIDEO_ENCODER_FACTORY(MPPFinalEncoder)
const char *FACTORY(MPPFinalEncoder)::ExpectedInputDataType() {
  return MppAcceptImageFmts();
}
const char *FACTORY(MPPFinalEncoder)::OutPutDataType() { return VIDEO_H264; }

} // namespace easymedia
