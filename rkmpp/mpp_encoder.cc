/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "mpp_encoder.h"

#include <string.h>
#include <unistd.h>

#include <mpp/mpp_log.h>

#include "buffer.h"

namespace rkmedia {

MPPEncoder::MPPEncoder()
    : coding_type(MPP_VIDEO_CodingAutoDetect), ctx(nullptr), mpi(nullptr) {}

MPPEncoder::~MPPEncoder() {
  if (mpi) {
    mpi->reset(ctx);
    mpp_destroy(ctx);
    ctx = NULL;
  }
}

bool MPPEncoder::Init() {
  int ret = mpp_create(&ctx, &mpi);
  if (ret) {
    LOG("mpp_create failed");
    return false;
  }
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

bool MPPEncoder::InitConfig(const MediaConfig &cfg) {
  return VideoEncoder::InitConfig(cfg);
}

static int init_mpp_buffer(MppBuffer &buffer, std::shared_ptr<MediaBuffer> &mb,
                           size_t frame_size) {
  int ret;
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
      return 0;
    if (size == 0) {
      size = frame_size;
      mpp_assert(frame_size > 0);
    }
    ret = mpp_buffer_get(NULL, &buffer, size);
    if (ret) {
      LOG("allocate output stream buffer failed\n");
      goto fail;
    }
  }

  return 0;

fail:
  if (buffer) {
    mpp_buffer_put(buffer);
    buffer = nullptr;
  }
  return ret;
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
  int fd = input->GetFD();
  void *ptr = input->GetPtr();
  size_t size = input->GetValidSize();

  mpp_assert(size > 0);
  mpp_frame_set_pts(frame, hw_buffer->GetTimeStamp());
  mpp_frame_set_dts(frame, hw_buffer->GetTimeStamp());
  mpp_frame_set_width(frame, hw_buffer->GetWidth());
  mpp_frame_set_height(frame, hw_buffer->GetHeight());
  mpp_frame_set_fmt(frame, ConvertToMppPixFmt(fmt));

  if (fmt == PIX_FMT_YUYV422 || fmt == PIX_FMT_UYVY422)
    mpp_frame_set_hor_stride(frame, hw_buffer->GetVirWidth() * 2);
  else
    mpp_frame_set_hor_stride(frame, hw_buffer->GetVirWidth());
  mpp_frame_set_ver_stride(frame, hw_buffer->GetVirHeight());

  int ret = init_mpp_buffer(pic_buf, input, size);
  if (ret) {
    LOG("prepare picture buffer failed\n");
    return ret;
  }
  if (fd < 0 && ptr) {
    // !!time-consuming operation
    LOG("extra time-consuming memcpy to mpp!\n");
    memcpy(mpp_buffer_get_ptr(pic_buf), ptr, size);
    // sync to device?
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

  int ret = init_mpp_buffer(mpp_buf, output, 0);
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
  int ret =
      init_mpp_buffer(mpp_buf, extra_output, extra_output->GetValidSize());
  if (ret) {
    LOG("import extra stream buffer failed\n");
    return ret;
  }
  buffer = mpp_buf;
  return 0;
}

static int free_mpp_packet(MppPacket packet) {
  if (packet)
    mpp_packet_deinit(&packet);
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
      mpp_assert(ptr);
      LOG("extra time-consuming memcpy to cpu!\n");
      memcpy(ptr, mpp_packet_get_data(packet), packet_len);
      // sync to cpu?
    }
  } else {
    output->SetFD(mpp_buffer_get_fd(mpp_packet_get_buffer(packet)));
    output->SetPtr(mpp_packet_get_data(packet));
    output->SetSize(mpp_packet_get_size(packet));
    output->SetUserData(packet, free_mpp_packet);
    packet = nullptr;
  }
  output->SetValidSize(packet_len);
  output->SetUserFlag(packet_flag);
  output->SetTimeStamp(pts);
  output->SetEOF(out_eof ? true : false);

  if (mv_buf) {
    if (extra_output->GetFD() < 0) {
      void *ptr = extra_output->GetPtr();
      mpp_assert(ptr);
      memcpy(ptr, mpp_buffer_get_ptr(mv_buf), mpp_buffer_get_size(mv_buf));
    }
    extra_output->SetValidSize(mpp_buffer_get_size(mv_buf));
    extra_output->SetUserFlag(packet_flag);
    extra_output->SetTimeStamp(pts);
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
  MppTask task = nullptr;

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
      mpp_assert(packet == packet_out);
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
  int ret = mpi->control(ctx, mpi_cmd, (MppParam)param);

  if (ret) {
    LOG("mpp control ctx %p cmd 0x%08x param %p failed\n", ctx, cmd, param);
    return ret;
  }

  return 0;
}

} // namespace rkmedia
