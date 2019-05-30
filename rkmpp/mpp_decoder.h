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

#ifndef RKMEDIA_MPP_DECODER_H
#define RKMEDIA_MPP_DECODER_H

#include "decoder.h"
#include "mpp_inc.h"
#include <mpp/rk_mpi.h>

namespace easymedia {

// A hw video decoder which call the mpp interface directly.
class MPPDecoder : public VideoDecoder {
public:
  MPPDecoder(const char *param);
  virtual ~MPPDecoder();
  static const char *GetCodecName() { return "rkmpp"; }

  virtual bool Init() override;
  virtual int Process(std::shared_ptr<MediaBuffer> input,
                      std::shared_ptr<MediaBuffer> output,
                      std::shared_ptr<MediaBuffer> extra_output) override;
  virtual int SendInput(std::shared_ptr<MediaBuffer> input) override;
  virtual std::shared_ptr<MediaBuffer> FetchOutput() override;

private:
  std::string input_data_type;
  RK_S32 fg_limit_num;
  RK_U32 need_split;
  RK_U32 timeout;
  MppCodingType coding_type;
  MppCtx ctx;
  MppApi *mpi;
  MppBufferGroup frame_group;
  bool support_sync;
  static const RK_S32 kFRAMEGROUP_MAX_FRAMES = 16;
};

} // namespace easymedia

#endif // RKMEDIA_MPP_DECODER_H
