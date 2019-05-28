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

#ifndef RKMEDIA_MPP_ENCODER_H
#define RKMEDIA_MPP_ENCODER_H

#include "encoder.h"
#include "mpp_inc.h"
#include <mpp/rk_mpi.h>

// mpp_packet_impl.h which define MPP_PACKET_FLAG_INTRA is not exposed,
// here define the same MPP_PACKET_FLAG_INTRA.
#ifndef MPP_PACKET_FLAG_INTRA
#define MPP_PACKET_FLAG_INTRA (0x00000008)
#endif

namespace rkmedia {
// A encoder which call the mpp interface directly.
// Mpp is always video process module.
class MPPEncoder : public VideoEncoder {
public:
  MPPEncoder();
  virtual ~MPPEncoder();

  virtual bool Init() override;
  virtual bool InitConfig(const MediaConfig &cfg) override;

  // sync encode the raw input buffer to output buffer
  virtual int Process(std::shared_ptr<MediaBuffer> input,
                      std::shared_ptr<MediaBuffer> output,
                      std::shared_ptr<MediaBuffer> extra_output) override;

  virtual int SendInput(std::shared_ptr<MediaBuffer> input _UNUSED) override;
  virtual std::shared_ptr<MediaBuffer> FetchOutput() override;

protected:
  // call before Init()
  void SetMppCodeingType(MppCodingType type) { coding_type = type; }
  virtual bool
  CheckConfigChange(std::pair<uint32_t, std::shared_ptr<ParameterBuffer>>) {
    return true;
  }
  // Control before encoding.
  int EncodeControl(int cmd, void *param);

  virtual int PrepareMppFrame(std::shared_ptr<MediaBuffer> input,
                              MppFrame &frame);
  virtual int PrepareMppPacket(std::shared_ptr<MediaBuffer> output,
                               MppPacket &packet);
  virtual int PrepareMppExtraBuffer(std::shared_ptr<MediaBuffer> extra_output,
                                    MppBuffer &buffer);
  int Process(MppFrame frame, MppPacket &packet, MppBuffer &mv_buf);

private:
  MppCodingType coding_type;
  MppCtx ctx;
  MppApi *mpi;
};

} // namespace rkmedia

#endif // MPP_H264_ENCODER_H
