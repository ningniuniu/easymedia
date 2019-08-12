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

#include "mpp_encoder.h"

#include <assert.h>
#include <unistd.h>

#include "buffer.h"

namespace easymedia {

MPPEncoder::MPPEncoder()
    : coding_type(MPP_VIDEO_CodingAutoDetect), output_mb_flags(0),
      output_fmt(PIX_FMT_NONE) {}

void MPPEncoder::SetMppCodeingType(MppCodingType type) {
  coding_type = type;
  if (type == MPP_VIDEO_CodingMJPEG)
    output_fmt = PIX_FMT_JPEG;
  else if (type == MPP_VIDEO_CodingAVC)
    output_fmt = PIX_FMT_H264;
  else if (type == MPP_VIDEO_CodingHEVC)
    output_fmt = PIX_FMT_H265;
  // mpp always return a single nal
  if (type == MPP_VIDEO_CodingAVC || type == MPP_VIDEO_CodingHEVC)
    output_mb_flags |= MediaBuffer::kSingleNalUnit;
}

bool MPPEncoder::Init() {
  if (coding_type == MPP_VIDEO_CodingUnused)
    return false;
  mpp_ctx = std::make_shared<MPPContext>();
  if (!mpp_ctx)
    return false;
  MppCtx ctx = NULL;
  MppApi *mpi = NULL;
  int ret = mpp_create(&ctx, &mpi);
  if (ret) {
    LOG("mpp_create failed\n");
    return false;
  }
  mpp_ctx->ctx = ctx;
  mpp_ctx->mpi = mpi;
  ret = mpp_init(ctx, MPP_CTX_ENC, coding_type);
  if (ret != MPP_OK) {
    LOG("mpp_init failed with type %d\n", coding_type);
    mpp_destroy(ctx);
    ctx = NULL;
    mpi = NULL;
    return false;
  }
  return true;
}

int MPPEncoder::PrepareMppFrame(std::shared_ptr<MediaBuffer> input,
                                MppFrame &frame) {
  MppBuffer pic_buf = nullptr;
  if (input->GetType() != Type::Image) {
    LOG("mpp encoder input source only support image buffer\n");
    return -EINVAL;
  }
  PixelFormat fmt = input->GetPixelFormat();
  if (fmt == PIX_FMT_NONE) {
    LOG("mpp encoder input source invalid pixel format\n");
    return -EINVAL;
  }
  ImageBuffer *hw_buffer = static_cast<ImageBuffer *>(input.get());

  assert(input->GetValidSize() > 0);
  mpp_frame_set_pts(frame, hw_buffer->GetUSTimeStamp());
  mpp_frame_set_dts(frame, hw_buffer->GetUSTimeStamp());
  mpp_frame_set_width(frame, hw_buffer->GetWidth());
  mpp_frame_set_height(frame, hw_buffer->GetHeight());
  mpp_frame_set_fmt(frame, ConvertToMppPixFmt(fmt));

  if (fmt == PIX_FMT_YUYV422 || fmt == PIX_FMT_UYVY422)
    mpp_frame_set_hor_stride(frame, hw_buffer->GetVirWidth() * 2);
  else
    mpp_frame_set_hor_stride(frame, hw_buffer->GetVirWidth());
  mpp_frame_set_ver_stride(frame, hw_buffer->GetVirHeight());

  MPP_RET ret = init_mpp_buffer_with_content(pic_buf, input);
  if (ret) {
    LOG("prepare picture buffer failed\n");
    return ret;
  }

  mpp_frame_set_buffer(frame, pic_buf);
  if (input->IsEOF())
    mpp_frame_set_eos(frame, 1);

  mpp_buffer_put(pic_buf);

  return 0;
}

int MPPEncoder::PrepareMppPacket(std::shared_ptr<MediaBuffer> output,
                                 MppPacket &packet) {
  MppBuffer mpp_buf = nullptr;

  if (!output->IsHwBuffer())
    return 0;

  MPP_RET ret = init_mpp_buffer(mpp_buf, output, 0);
  if (ret) {
    LOG("import output stream buffer failed\n");
    return ret;
  }

  if (mpp_buf) {
    mpp_packet_init_with_buffer(&packet, mpp_buf);
    mpp_buffer_put(mpp_buf);
  }

  return 0;
}

int MPPEncoder::PrepareMppExtraBuffer(std::shared_ptr<MediaBuffer> extra_output,
                                      MppBuffer &buffer) {
  MppBuffer mpp_buf = nullptr;
  if (!extra_output || !extra_output->IsValid())
    return 0;
  MPP_RET ret =
      init_mpp_buffer(mpp_buf, extra_output, extra_output->GetValidSize());
  if (ret) {
    LOG("import extra stream buffer failed\n");
    return ret;
  }
  buffer = mpp_buf;
  return 0;
}

class MPPPacketContext {
public:
  MPPPacketContext(std::shared_ptr<MPPContext> ctx, MppPacket p)
      : mctx(ctx), packet(p) {}
  ~MPPPacketContext() {
    if (packet)
      mpp_packet_deinit(&packet);
  }

private:
  std::shared_ptr<MPPContext> mctx;
  MppPacket packet;
};

static int __free_mpppacketcontext(void *p) {
  assert(p);
  delete (MPPPacketContext *)p;
  return 0;
}

int MPPEncoder::Process(std::shared_ptr<MediaBuffer> input,
                        std::shared_ptr<MediaBuffer> output,
                        std::shared_ptr<MediaBuffer> extra_output) {
  MppFrame frame = nullptr;
  MppPacket packet = nullptr;
  MppPacket import_packet = nullptr;
  MppBuffer mv_buf = nullptr;
  size_t packet_len = 0;
  RK_U32 packet_flag = 0;
  RK_U32 out_eof = 0;
  RK_S64 pts = 0;

  Type out_type;

  if (!input)
    return 0;
  if (!output)
    return -EINVAL;

  // all changes must set before encode and among the same thread
  if (HasChangeReq()) {
    auto &&change = PeekChange();
    if (change.first && !CheckConfigChange(change))
      return -1;
  }

  int ret = mpp_frame_init(&frame);
  if (MPP_OK != ret) {
    LOG("mpp_frame_init failed\n");
    goto ENCODE_OUT;
  }

  ret = PrepareMppFrame(input, frame);
  if (ret) {
    LOG("PrepareMppFrame failed\n");
    goto ENCODE_OUT;
  }

  if (output->IsValid()) {
    ret = PrepareMppPacket(output, packet);
    if (ret) {
      LOG("PrepareMppPacket failed\n");
      goto ENCODE_OUT;
    }
    import_packet = packet;
  }

  ret = PrepareMppExtraBuffer(extra_output, mv_buf);
  if (ret) {
    LOG("PrepareMppExtraBuffer failed\n");
    goto ENCODE_OUT;
  }

  ret = Process(frame, packet, mv_buf);
  if (ret)
    goto ENCODE_OUT;

  packet_len = mpp_packet_get_length(packet);
  packet_flag = (mpp_packet_get_flag(packet) & MPP_PACKET_FLAG_INTRA)
                    ? MediaBuffer::kIntra
                    : MediaBuffer::kPredicted;
  out_eof = mpp_packet_get_eos(packet);
  pts = mpp_packet_get_pts(packet);
  if (pts <= 0)
    pts = mpp_packet_get_dts(packet);
  if (output->IsValid()) {
    if (!import_packet) {
      // !!time-consuming operation
      void *ptr = output->GetPtr();
      assert(ptr);
      LOGD("extra time-consuming memcpy to cpu!\n");
      memcpy(ptr, mpp_packet_get_data(packet), packet_len);
      // sync to cpu?
    }
  } else {
    MPPPacketContext *ctx = new MPPPacketContext(mpp_ctx, packet);
    if (!ctx) {
      LOG_NO_MEMORY();
      ret = -ENOMEM;
      goto ENCODE_OUT;
    }
    output->SetFD(mpp_buffer_get_fd(mpp_packet_get_buffer(packet)));
    output->SetPtr(mpp_packet_get_data(packet));
    output->SetSize(mpp_packet_get_size(packet));
    output->SetUserData(ctx, __free_mpppacketcontext);
    packet = nullptr;
  }
  output->SetValidSize(packet_len);
  output->SetUserFlag(packet_flag | output_mb_flags);
  output->SetUSTimeStamp(pts);
  output->SetEOF(out_eof ? true : false);
  out_type = output->GetType();
  if (out_type == Type::Image) {
    auto out_img = std::static_pointer_cast<ImageBuffer>(output);
    auto &info = out_img->GetImageInfo();
    const auto &in_cfg = GetConfig();
    info = (coding_type == MPP_VIDEO_CodingMJPEG)
               ? in_cfg.img_cfg.image_info
               : in_cfg.vid_cfg.image_cfg.image_info;
    info.pix_fmt = output_fmt;
  } else {
    output->SetType(Type::Video);
  }

  if (mv_buf) {
    if (extra_output->GetFD() < 0) {
      void *ptr = extra_output->GetPtr();
      assert(ptr);
      memcpy(ptr, mpp_buffer_get_ptr(mv_buf), mpp_buffer_get_size(mv_buf));
    }
    extra_output->SetValidSize(mpp_buffer_get_size(mv_buf));
    extra_output->SetUserFlag(packet_flag);
    extra_output->SetUSTimeStamp(pts);
  }

ENCODE_OUT:
  if (frame)
    mpp_frame_deinit(&frame);
  if (packet)
    mpp_packet_deinit(&packet);
  if (mv_buf)
    mpp_buffer_put(mv_buf);

  return ret;
}

int MPPEncoder::Process(MppFrame frame, MppPacket &packet, MppBuffer &mv_buf) {
  MppTask task = NULL;
  MppCtx ctx = mpp_ctx->ctx;
  MppApi *mpi = mpp_ctx->mpi;
  int ret = mpi->poll(ctx, MPP_PORT_INPUT, MPP_POLL_BLOCK);
  if (ret) {
    LOG("input poll ret %d\n", ret);
    return ret;
  }

  ret = mpi->dequeue(ctx, MPP_PORT_INPUT, &task);
  if (ret || NULL == task) {
    LOG("mpp task input dequeue failed\n");
    return ret;
  }

  mpp_task_meta_set_frame(task, KEY_INPUT_FRAME, frame);
  if (packet)
    mpp_task_meta_set_packet(task, KEY_OUTPUT_PACKET, packet);
  if (mv_buf)
    mpp_task_meta_set_buffer(task, KEY_MOTION_INFO, mv_buf);
  ret = mpi->enqueue(ctx, MPP_PORT_INPUT, task);
  if (ret) {
    LOG("mpp task input enqueue failed\n");
    return ret;
  }
  task = NULL;

  ret = mpi->poll(ctx, MPP_PORT_OUTPUT, MPP_POLL_BLOCK);
  if (ret) {
    LOG("output poll ret %d\n", ret);
    return ret;
  }

  ret = mpi->dequeue(ctx, MPP_PORT_OUTPUT, &task);
  if (ret || !task) {
    LOG("mpp task output dequeue failed, ret %d \n", ret);
    return ret;
  }
  if (task) {
    MppPacket packet_out = nullptr;
    mpp_task_meta_get_packet(task, KEY_OUTPUT_PACKET, &packet_out);
    ret = mpi->enqueue(ctx, MPP_PORT_OUTPUT, task);
    if (ret != MPP_OK) {
      LOG("enqueue task output failed, ret = %d\n", ret);
      return ret;
    }
    if (!packet) {
      // the buffer comes from mpp
      packet = packet_out;
      packet_out = nullptr;
    } else {
      assert(packet == packet_out);
    }
    // should not go follow
    if (packet_out && packet != packet_out) {
      mpp_packet_deinit(&packet);
      packet = packet_out;
    }
  }

  return 0;
}

int MPPEncoder::SendInput(std::shared_ptr<MediaBuffer> input _UNUSED) {
  errno = ENOSYS;
  return -1;
}
std::shared_ptr<MediaBuffer> MPPEncoder::FetchOutput() {
  errno = ENOSYS;
  return nullptr;
}

int MPPEncoder::EncodeControl(int cmd, void *param) {
  MpiCmd mpi_cmd = (MpiCmd)cmd;
  int ret = mpp_ctx->mpi->control(mpp_ctx->ctx, mpi_cmd, (MppParam)param);

  if (ret) {
    LOG("mpp control cmd 0x%08x param %p failed\n", cmd, param);
    return ret;
  }

  return 0;
}

} // namespace easymedia
