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

#include "mpp_inc.h"

#include <assert.h>

#include "media_config.h"
#include "media_type.h"
#include "utils.h"

namespace easymedia {
MppFrameFormat ConvertToMppPixFmt(const PixelFormat &fmt) {
  // static const MppFrameFormat invalid_mpp_fmt =
  // static_cast<MppFrameFormat>(-1);
  static_assert(PIX_FMT_YUV420P == 0, "The index should greater than 0\n");
  static MppFrameFormat mpp_fmts[PIX_FMT_NB] = {
      [PIX_FMT_YUV420P] = MPP_FMT_YUV420P,
      [PIX_FMT_NV12] = MPP_FMT_YUV420SP,
      [PIX_FMT_NV21] = MPP_FMT_YUV420SP_VU,
      [PIX_FMT_YUV422P] = MPP_FMT_YUV422P,
      [PIX_FMT_NV16] = MPP_FMT_YUV422SP,
      [PIX_FMT_NV61] = MPP_FMT_YUV422SP_VU,
      [PIX_FMT_YUYV422] = MPP_FMT_YUV422_YUYV,
      [PIX_FMT_UYVY422] = MPP_FMT_YUV422_UYVY,
      [PIX_FMT_RGB332] = (MppFrameFormat)-1,
      [PIX_FMT_RGB565] = MPP_FMT_RGB565,
      [PIX_FMT_BGR565] = MPP_FMT_BGR565,
      [PIX_FMT_RGB888] = MPP_FMT_RGB888,
      [PIX_FMT_BGR888] = MPP_FMT_BGR888,
      [PIX_FMT_ARGB8888] = MPP_FMT_ARGB8888,
      [PIX_FMT_ABGR8888] = MPP_FMT_ABGR8888};
  if (fmt >= 0 && fmt < PIX_FMT_NB)
    return mpp_fmts[fmt];
  return (MppFrameFormat)-1;
}

PixelFormat ConvertToPixFmt(const MppFrameFormat &mfmt) {
  switch (mfmt) {
  case MPP_FMT_YUV420P:
    return PIX_FMT_YUV420P;
  case MPP_FMT_YUV420SP:
    return PIX_FMT_NV12;
  case MPP_FMT_YUV420SP_VU:
    return PIX_FMT_NV21;
  case MPP_FMT_YUV422P:
    return PIX_FMT_YUV422P;
  case MPP_FMT_YUV422SP:
    return PIX_FMT_NV16;
  case MPP_FMT_YUV422SP_VU:
    return PIX_FMT_NV61;
  case MPP_FMT_YUV422_YUYV:
    return PIX_FMT_YUYV422;
  case MPP_FMT_YUV422_UYVY:
    return PIX_FMT_UYVY422;
  case MPP_FMT_RGB565:
    return PIX_FMT_RGB565;
  case MPP_FMT_BGR565:
    return PIX_FMT_BGR565;
  case MPP_FMT_RGB888:
    return PIX_FMT_RGB888;
  case MPP_FMT_BGR888:
    return PIX_FMT_BGR888;
  case MPP_FMT_ARGB8888:
    return PIX_FMT_ARGB8888;
  case MPP_FMT_ABGR8888:
    return PIX_FMT_ABGR8888;
  default:
    LOG("unsupport for mpp pixel fmt: %d\n", mfmt);
    return PIX_FMT_NONE;
  }
}

class _MPP_SUPPORT_TYPES : public SupportMediaTypes {
public:
  _MPP_SUPPORT_TYPES() {
    types.append(TYPENEAR(IMAGE_YUV420P));
    types.append(TYPENEAR(IMAGE_NV12));
    types.append(TYPENEAR(IMAGE_NV21));
    types.append(TYPENEAR(IMAGE_YUV422P));
    types.append(TYPENEAR(IMAGE_NV16));
    types.append(TYPENEAR(IMAGE_NV61));
    types.append(TYPENEAR(IMAGE_YUYV422));
    types.append(TYPENEAR(IMAGE_UYVY422));
    types.append(TYPENEAR(IMAGE_RGB565));
    types.append(TYPENEAR(IMAGE_BGR565));
    types.append(TYPENEAR(IMAGE_RGB888));
    types.append(TYPENEAR(IMAGE_BGR888));
    types.append(TYPENEAR(IMAGE_ARGB8888));
    types.append(TYPENEAR(IMAGE_ABGR8888));
  }
};
static _MPP_SUPPORT_TYPES priv_types;

const char *MppAcceptImageFmts() { return priv_types.types.c_str(); }

MppCodingType GetMPPCodingType(const std::string &data_type) {
  if (data_type == VIDEO_H264)
    return MPP_VIDEO_CodingAVC;
  if (data_type == VIDEO_H265)
    return MPP_VIDEO_CodingHEVC;
  else if (data_type == IMAGE_JPEG)
    return MPP_VIDEO_CodingMJPEG;
  LOG("mpp decoder TODO for %s\n", data_type.c_str());
  return MPP_VIDEO_CodingUnused;
}

static MppEncRcQuality mpp_rc_qualitys[] = {
    MPP_ENC_RC_QUALITY_WORST,  MPP_ENC_RC_QUALITY_WORSE,
    MPP_ENC_RC_QUALITY_MEDIUM, MPP_ENC_RC_QUALITY_BETTER,
    MPP_ENC_RC_QUALITY_BEST,   MPP_ENC_RC_QUALITY_CQP,
    MPP_ENC_RC_QUALITY_AQ_ONLY};

MppEncRcQuality GetMPPRCQuality(const char *quality) {
  static_assert(ARRAY_ELEMS(rc_quality_strings) == ARRAY_ELEMS(mpp_rc_qualitys),
                "rc quality strings miss match mpp rc quality enum");
  if (!quality)
    return MPP_ENC_RC_QUALITY_BUTT;
  for (size_t i = 0; i < ARRAY_ELEMS(rc_quality_strings); i++)
    if (!strcasecmp(quality, rc_quality_strings[i]))
      return mpp_rc_qualitys[i];

  return MPP_ENC_RC_QUALITY_BUTT;
}

static const MppEncRcMode mpp_rc_modes[] = {MPP_ENC_RC_MODE_VBR,
                                            MPP_ENC_RC_MODE_CBR};

MppEncRcMode GetMPPRCMode(const char *rc_mode) {
  static_assert(ARRAY_ELEMS(rc_mode_strings) == ARRAY_ELEMS(mpp_rc_modes),
                "rc mode strings miss match mpp rc mode enum");
  if (!rc_mode)
    return MPP_ENC_RC_MODE_BUTT;
  for (size_t i = 0; i < ARRAY_ELEMS(rc_mode_strings); i++)
    if (!strcasecmp(rc_mode, rc_mode_strings[i]))
      return mpp_rc_modes[i];

  return MPP_ENC_RC_MODE_BUTT;
}

MPPContext::MPPContext() : ctx(NULL), mpi(NULL), frame_group(NULL) {}
MPPContext::~MPPContext() {
  if (mpi) {
    mpi->reset(ctx);
    mpp_destroy(ctx);
    ctx = NULL;
    LOG("mpp destroy ctx done\n");
  }
  if (frame_group) {
    mpp_buffer_group_put(frame_group);
    frame_group = NULL;
    LOG("mpp buffer group free done\n");
  }
}

MPP_RET init_mpp_buffer(MppBuffer &buffer,
                        const std::shared_ptr<MediaBuffer> &mb,
                        size_t frame_size) {
  MPP_RET ret;
  int fd = mb->GetFD();
  void *ptr = mb->GetPtr();
  size_t size = mb->GetValidSize();

  if (fd >= 0) {
    MppBufferInfo info;

    memset(&info, 0, sizeof(info));
    info.type = MPP_BUFFER_TYPE_ION;
    info.size = size;
    info.fd = fd;
    info.ptr = ptr;

    ret = mpp_buffer_import(&buffer, &info);
    if (ret) {
      LOG("import input picture buffer failed\n");
      goto fail;
    }
  } else {
    if (frame_size == 0)
      return MPP_OK;
    if (size == 0) {
      size = frame_size;
      assert(frame_size > 0);
    }
    ret = mpp_buffer_get(NULL, &buffer, size);
    if (ret) {
      LOG("allocate output stream buffer failed\n");
      goto fail;
    }
  }

  return MPP_OK;

fail:
  if (buffer) {
    mpp_buffer_put(buffer);
    buffer = nullptr;
  }
  return ret;
}

MPP_RET init_mpp_buffer_with_content(MppBuffer &buffer,
                                     const std::shared_ptr<MediaBuffer> &mb) {
  size_t size = mb->GetValidSize();
  MPP_RET ret = init_mpp_buffer(buffer, mb, size);
  if (ret)
    return ret;
  int fd = mb->GetFD();
  void *ptr = mb->GetPtr();
  // As init_mpp_buffer is a no time-consuming function and do not memcpy
  // content to a virtual buffer, do memcpy here.
  if (fd < 0 && ptr) {
    // !!time-consuming operation
    // LOGD("extra time-consuming memcpy to mpp, size=%d!\n", size);
    memcpy(mpp_buffer_get_ptr(buffer), ptr, size);
    // sync between cpu and device if cached?
  }
  return MPP_OK;
}

} // namespace easymedia
