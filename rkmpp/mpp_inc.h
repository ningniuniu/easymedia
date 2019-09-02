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

#ifndef EASYMEDIA_MPP_INC_H_
#define EASYMEDIA_MPP_INC_H_

#include "buffer.h"
#include "image.h"
#include <rk_mpi.h>

namespace easymedia {
// mpp_packet_impl.h which define MPP_PACKET_FLAG_INTRA is not exposed,
// here define the same MPP_PACKET_FLAG_INTRA.
#ifndef MPP_PACKET_FLAG_INTRA
#define MPP_PACKET_FLAG_INTRA (0x00000008)
#endif

MppFrameFormat ConvertToMppPixFmt(const PixelFormat &fmt);
PixelFormat ConvertToPixFmt(const MppFrameFormat &mfmt);
const char *MppAcceptImageFmts();
MppCodingType GetMPPCodingType(const std::string &data_type);
MppEncRcQuality GetMPPRCQuality(const char *quality);
MppEncRcMode GetMPPRCMode(const char *rc_mode);

struct MPPContext {
  MPPContext();
  ~MPPContext();
  MppCtx ctx;
  MppApi *mpi;
  MppBufferGroup frame_group;
};

// no time-consuming, init a mppbuffer with MediaBuffer
MPP_RET init_mpp_buffer(MppBuffer &buffer,
                        const std::shared_ptr<MediaBuffer> &mb,
                        size_t frame_size);
// may time-consuming
MPP_RET init_mpp_buffer_with_content(MppBuffer &buffer,
                                     const std::shared_ptr<MediaBuffer> &mb);
} // namespace easymedia

#endif // EASYMEDIA_MPP_INC_H_
