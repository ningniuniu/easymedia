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

#include "rga.h"

#include <vector>

#include <rga/RockchipRga.h>

#include "buffer.h"
#include "filter.h"

namespace easymedia {

class RgaFilter : public Filter {
public:
  RgaFilter(const char *param);
  virtual ~RgaFilter() = default;
  static const char *GetFilterName() { return "rkrga"; }
  virtual int Process(std::shared_ptr<MediaBuffer> input,
                      std::shared_ptr<MediaBuffer> output) override;
  virtual int SendInput(std::shared_ptr<MediaBuffer> input _UNUSED) final {
    errno = ENOSYS;
    return -1;
  }
  virtual std::shared_ptr<MediaBuffer> FetchOutput() final {
    errno = ENOSYS;
    return nullptr;
  }

  static RockchipRga gRkRga;

private:
  std::vector<ImageRect> vec_rect;
  int rotate;
};

RockchipRga RgaFilter::gRkRga;

RgaFilter::RgaFilter(const char *param) : rotate(0) {
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params)) {
    SetError(-EINVAL);
    return;
  }
  const std::string &value = params[KEY_BUFFER_RECT];
  auto &&rects = StringToTwoImageRect(value);
  if (rects.empty()) {
    LOG("missing rects\n");
    SetError(-EINVAL);
    return;
  }
  vec_rect = std::move(rects);
  const std::string &v = params[KEY_BUFFER_ROTATE];
  if (!v.empty())
    rotate = std::stoi(v);
}

int RgaFilter::Process(std::shared_ptr<MediaBuffer> input,
                       std::shared_ptr<MediaBuffer> output) {
  if (vec_rect.size() < 2)
    return -EINVAL;
  if (!input || input->GetType() != Type::Image)
    return -EINVAL;
  if (!output || (output && output->GetType() != Type::Image))
    return -EINVAL;
  return rga_blit(std::static_pointer_cast<easymedia::ImageBuffer>(input),
                  std::static_pointer_cast<easymedia::ImageBuffer>(output),
                  &vec_rect[0], &vec_rect[1], rotate);
}

static int get_rga_format(PixelFormat f) {
  static std::map<PixelFormat, int> rga_format_map = {
      {PIX_FMT_YUV420P, RK_FORMAT_YCrCb_420_P},
      {PIX_FMT_NV12, RK_FORMAT_YCrCb_420_SP},
      {PIX_FMT_NV21, RK_FORMAT_YCbCr_420_SP},
      {PIX_FMT_YUV422P, RK_FORMAT_YCrCb_422_P},
      {PIX_FMT_NV16, RK_FORMAT_YCrCb_422_SP},
      {PIX_FMT_NV61, RK_FORMAT_YCbCr_422_SP},
      {PIX_FMT_YUYV422, -1},
      {PIX_FMT_UYVY422, -1},
      {PIX_FMT_RGB565, RK_FORMAT_RGB_565},
      {PIX_FMT_BGR565, -1},
      {PIX_FMT_RGB888, RK_FORMAT_RGB_888},
      {PIX_FMT_BGR888, RK_FORMAT_BGR_888},
      {PIX_FMT_ARGB8888, RK_FORMAT_RGBA_8888},
      {PIX_FMT_ABGR8888, RK_FORMAT_BGRA_8888}};
  auto it = rga_format_map.find(f);
  if (it != rga_format_map.end())
    return it->second;
  return -1;
}

int rga_blit(std::shared_ptr<ImageBuffer> src, std::shared_ptr<ImageBuffer> dst,
             ImageRect *src_rect, ImageRect *dst_rect, int rotate) {
  if (!src || !src->IsValid())
    return -EINVAL;
  if (!dst || !dst->IsValid())
    return -EINVAL;
  rga_info_t src_info, dst_info;
  memset(&src_info, 0, sizeof(src_info));
  src_info.fd = src->GetFD();
  if (src_info.fd < 0)
    src_info.virAddr = src->GetPtr();
  src_info.mmuFlag = 1;
  src_info.rotation = rotate;
  if (src_rect)
    rga_set_rect(&src_info.rect, src_rect->x, src_rect->y, src_rect->w,
                 src_rect->h, src->GetVirWidth(), src->GetVirHeight(),
                 get_rga_format(src->GetPixelFormat()));
  else
    rga_set_rect(&src_info.rect, 0, 0, src->GetWidth(), src->GetHeight(),
                 src->GetVirWidth(), src->GetVirHeight(),
                 get_rga_format(src->GetPixelFormat()));

  memset(&dst_info, 0, sizeof(dst_info));
  dst_info.fd = src->GetFD();
  if (dst_info.fd < 0)
    dst_info.virAddr = dst->GetPtr();
  dst_info.mmuFlag = 1;
  if (dst_rect)
    rga_set_rect(&dst_info.rect, dst_rect->x, dst_rect->y, dst_rect->w,
                 dst_rect->h, dst->GetVirWidth(), dst->GetVirHeight(),
                 get_rga_format(dst->GetPixelFormat()));
  else
    rga_set_rect(&dst_info.rect, 0, 0, dst->GetWidth(), dst->GetHeight(),
                 dst->GetVirWidth(), dst->GetVirHeight(),
                 get_rga_format(dst->GetPixelFormat()));

  int ret = RgaFilter::gRkRga.RkRgaBlit(&src_info, &dst_info, NULL);
  if (ret)
    LOG("RkRgaBlit ret = %d\n", ret);

  return ret;
}

class _PRIVATE_SUPPORT_FMTS : public SupportMediaTypes {
public:
  _PRIVATE_SUPPORT_FMTS() {
    types.append(TYPENEAR(IMAGE_YUV420P));
    types.append(TYPENEAR(IMAGE_NV12));
    types.append(TYPENEAR(IMAGE_NV21));
    types.append(TYPENEAR(IMAGE_YUV422P));
    types.append(TYPENEAR(IMAGE_NV16));
    types.append(TYPENEAR(IMAGE_NV61));
    // types.append(TYPENEAR(IMAGE_YUYV422));
    // types.append(TYPENEAR(IMAGE_UYVY422));
    types.append(TYPENEAR(IMAGE_RGB565));
    types.append(TYPENEAR(IMAGE_BGR565));
    types.append(TYPENEAR(IMAGE_RGB888));
    types.append(TYPENEAR(IMAGE_BGR888));
    types.append(TYPENEAR(IMAGE_ARGB8888));
    types.append(TYPENEAR(IMAGE_ABGR8888));
  }
};
static _PRIVATE_SUPPORT_FMTS priv_fmts;

DEFINE_COMMON_FILTER_FACTORY(RgaFilter)
const char *FACTORY(RgaFilter)::ExpectedInputDataType() {
  return priv_fmts.types.c_str();
}
const char *FACTORY(RgaFilter)::OutPutDataType() {
  return priv_fmts.types.c_str();
}

} // namespace easymedia
