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

#ifndef EASYMEDIA_MPP_DECODER_H
#define EASYMEDIA_MPP_DECODER_H

#include "decoder.h"
#include "mpp_inc.h"

namespace easymedia {

// A hw video decoder which call the mpp interface directly.
class MPPDecoder : public VideoDecoder {
public:
  MPPDecoder(const char *param);
  virtual ~MPPDecoder() = default;
  static const char *GetCodecName() { return "rkmpp"; }

  virtual bool Init() override;
  virtual int Process(const std::shared_ptr<MediaBuffer> &input,
                      std::shared_ptr<MediaBuffer> &output,
                      std::shared_ptr<MediaBuffer> extra_output) override;
  virtual int SendInput(const std::shared_ptr<MediaBuffer> &input) override;
  virtual std::shared_ptr<MediaBuffer> FetchOutput() override;

private:
  PixelFormat output_format;
  RK_S32 fg_limit_num;
  RK_U32 need_split;
  MppPollType timeout;
  MppCodingType coding_type;
  std::shared_ptr<MPPContext> mpp_ctx;
  bool support_sync;
  bool support_async;
  static const RK_S32 kFRAMEGROUP_MAX_FRAMES = 16;
};

} // namespace easymedia

#endif // EASYMEDIA_MPP_DECODER_H
