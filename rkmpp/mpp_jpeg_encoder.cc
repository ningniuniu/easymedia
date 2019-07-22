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

#include "buffer.h"
#include "media_type.h"
#include "mpp_encoder.h"

namespace easymedia {

class MPPJpegEncoder : public MPPEncoder {
public:
  MPPJpegEncoder(const char *param _UNUSED) {
    SetMppCodeingType(MPP_VIDEO_CodingMJPEG);
  }
  virtual ~MPPJpegEncoder() = default;

  static const char *GetCodecName() { return "rkmpp_jpeg"; }

  virtual bool InitConfig(const MediaConfig &cfg) override;

protected:
  virtual bool CheckConfigChange(
      std::pair<uint32_t, std::shared_ptr<ParameterBuffer>>) override;
};

bool MPPJpegEncoder::InitConfig(const MediaConfig &cfg) {
  const ImageConfig &img_cfg = cfg.img_cfg;
  MppEncPrepCfg prep_cfg;
  MppEncCodecCfg codec_cfg;
  MppFrameFormat pic_type = ConvertToMppPixFmt(img_cfg.image_info.pix_fmt);
  if (pic_type == -1) {
    LOG("error input pixel format\n");
    return false;
  }

  memset(&prep_cfg, 0, sizeof(prep_cfg));
  prep_cfg.change =
      MPP_ENC_PREP_CFG_CHANGE_INPUT | MPP_ENC_PREP_CFG_CHANGE_FORMAT;
  prep_cfg.width = img_cfg.image_info.width;
  prep_cfg.height = img_cfg.image_info.height;
  if (pic_type == MPP_FMT_YUV422_YUYV || pic_type == MPP_FMT_YUV422_UYVY)
    prep_cfg.hor_stride = img_cfg.image_info.vir_width * 2;
  else
    prep_cfg.hor_stride = img_cfg.image_info.vir_width;
  prep_cfg.ver_stride = img_cfg.image_info.vir_height;
  prep_cfg.format = pic_type;
  LOG("encode w x h(%d[%d] x %d[%d])\n", prep_cfg.width, prep_cfg.hor_stride,
      prep_cfg.height, prep_cfg.ver_stride);
  int ret = EncodeControl(MPP_ENC_SET_PREP_CFG, &prep_cfg);
  if (ret) {
    LOG("mpi control enc set cfg failed\n");
    return false;
  }

  memset(&codec_cfg, 0, sizeof(codec_cfg));
  codec_cfg.coding = MPP_VIDEO_CodingMJPEG;
  codec_cfg.jpeg.change = MPP_ENC_JPEG_CFG_CHANGE_QP;
  codec_cfg.jpeg.quant = img_cfg.qp_init;
  if (codec_cfg.jpeg.quant < 1)
    codec_cfg.jpeg.quant = 1;
  if (codec_cfg.jpeg.quant > 10)
    codec_cfg.jpeg.quant = 10;

  ret = EncodeControl(MPP_ENC_SET_CODEC_CFG, &codec_cfg);
  if (ret) {
    LOG("mpi control enc set codec cfg failed, ret=%d\n", ret);
    return false;
  }

  return MPPEncoder::InitConfig(cfg);
}

bool MPPJpegEncoder::CheckConfigChange(
    std::pair<uint32_t, std::shared_ptr<ParameterBuffer>> change_pair) {
  int ret;
  ImageConfig &iconfig = GetConfig().img_cfg;
  uint32_t change = change_pair.first;
  std::shared_ptr<ParameterBuffer> &val = change_pair.second;
  if (change & kQPChange) {
    MppEncCodecCfg codec_cfg;
    int quant = val->GetValue();

    memset(&codec_cfg, 0, sizeof(codec_cfg));
    codec_cfg.coding = MPP_VIDEO_CodingMJPEG;
    codec_cfg.jpeg.change = MPP_ENC_JPEG_CFG_CHANGE_QP;

    if (quant < 1)
      quant = 1;
    if (quant > 10)
      quant = 10;
    codec_cfg.jpeg.quant = quant;

    ret = EncodeControl(MPP_ENC_SET_CODEC_CFG, &codec_cfg);
    if (ret) {
      LOG("mpi control enc change codec cfg failed, ret=%d\n", ret);
      return false;
    }

    iconfig.qp_init = quant;
  } else {
    LOG("Unknown request change 0x%08x!\n", change);
    return false;
  }

  return true;
}

DEFINE_VIDEO_ENCODER_FACTORY(MPPJpegEncoder)
const char *FACTORY(MPPJpegEncoder)::ExpectedInputDataType() {
  return MppAcceptImageFmts();
}
const char *FACTORY(MPPJpegEncoder)::OutPutDataType() { return IMAGE_JPEG; }

} // namespace easymedia
