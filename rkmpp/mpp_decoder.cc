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

#include "mpp_decoder.h"

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <mpp/mpp_log.h>

#include "buffer.h"

namespace easymedia {

MPPDecoder::MPPDecoder(const char *param)
    : fg_limit_num(kFRAMEGROUP_MAX_FRAMES), need_split(1),
      timeout(MPP_POLL_NON_BLOCK), coding_type(MPP_VIDEO_CodingUnused),
      ctx(NULL), mpi(NULL), frame_group(NULL), support_sync(false) {
  MediaConfig &cfg = GetConfig();
  ImageInfo &img_info = cfg.img_cfg.image_info;
  img_info.pix_fmt = PIX_FMT_NONE;
  std::map<std::string, std::string> params;
  std::list<std::pair<const std::string, std::string &>> req_list;
  std::string limit_max_frame_num;
  std::string split_mode;
  std::string stimeout;
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_INPUTDATATYPE, input_data_type));
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_MPP_GROUP_MAX_FRAMES, limit_max_frame_num));
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_MPP_SPLIT_MODE, split_mode));
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_OUTPUT_TIMEOUT, stimeout));

  int ret = parse_media_param_match(param, params, req_list);
  if (ret == 0 || input_data_type.empty()) {
    LOG("missing %s\n", KEY_INPUTDATATYPE);
    errno = EINVAL;
    return;
  }
  if (!limit_max_frame_num.empty()) {
    fg_limit_num = std::stoi(limit_max_frame_num);
    if (fg_limit_num > 0 && fg_limit_num <= 3) {
      LOG("invalid framegroup limit frame num: %d\n", fg_limit_num);
      errno = EINVAL;
      return;
    }
  }
  if (!split_mode.empty())
    need_split = std::stoi(split_mode);
  if (!stimeout.empty()) {
    timeout = std::stoi(stimeout);
    if (timeout == 0)
      timeout = MPP_POLL_NON_BLOCK;
  }
}

MPPDecoder::~MPPDecoder() {
  if (mpi) {
    mpi->reset(ctx);
    mpp_destroy(ctx);
    ctx = NULL;
  }
  if (frame_group) {
    mpp_buffer_group_put(frame_group);
    frame_group = NULL;
  }
}

static MppCodingType get_codingtype(const std::string &data_type) {
  if (data_type == VIDEO_H264)
    return MPP_VIDEO_CodingAVC;
  if (data_type == IMAGE_JPEG)
    return MPP_VIDEO_CodingMJPEG;
  LOG("mpp decoder TODO for %s\n", data_type.c_str());
  return MPP_VIDEO_CodingUnused;
}

bool MPPDecoder::Init() {
  coding_type = get_codingtype(input_data_type);
  if (coding_type == MPP_VIDEO_CodingUnused)
    return false;
  if (coding_type == MPP_VIDEO_CodingMJPEG)
    support_sync = true;

  MPP_RET ret = mpp_create(&ctx, &mpi);
  if (MPP_OK != ret) {
    LOG("mpp_create failed\n");
    return false;
  }

  MppParam param = NULL;

  if (need_split) {
    param = &need_split;
    ret = mpi->control(ctx, MPP_DEC_SET_PARSER_SPLIT_MODE, param);
    LOG("mpi control MPP_DEC_SET_PARSER_SPLIT_MODE ret = %d\n", ret);
    if (MPP_OK != ret)
      return false;
  }

  ret = mpp_init(ctx, MPP_CTX_DEC, coding_type);
  if (ret != MPP_OK) {
    LOG("mpp_init dec failed with type %d\n", coding_type);
    return false;
  }

  if (timeout != MPP_POLL_NON_BLOCK) {
    param = &timeout;
    ret = mpi->control(ctx, MPP_SET_OUTPUT_TIMEOUT, param);
    LOG("mpi set output timeout = %d, ret = %d\n", timeout, ret);
    if (MPP_OK != ret)
      return false;
  }

  if (fg_limit_num > 0) {
    ret = mpp_buffer_group_get_internal(&frame_group, MPP_BUFFER_TYPE_ION);
    if (ret != MPP_OK) {
      LOG("Failed to retrieve buffer group (ret = %d)\n", ret);
      return false;
    }
    ret = mpi->control(ctx, MPP_DEC_SET_EXT_BUF_GROUP, frame_group);
    if (ret != MPP_OK) {
      LOG("Failed to assign buffer group (ret = %d)\n", ret);
      return false;
    }
    ret = mpp_buffer_group_limit_config(frame_group, 0, fg_limit_num);
    if (ret != MPP_OK) {
      LOG("Failed to set buffer group limit (ret = %d)\n", ret);
      return false;
    }
    LOG("mpi set group limit = %d\n", fg_limit_num);
  }

  return true;
}

static int __mpp_frame_deinit(void *p) {
  MppFrame mppframe = p;
  return mpp_frame_deinit(&mppframe);
}

// frame may be deinit here or depends on ImageBuffer
static int SetImageBufferWithMppFrame(std::shared_ptr<ImageBuffer> ib,
                                      MppFrame &frame) {
  const MppBuffer buffer = mpp_frame_get_buffer(frame);
  if (!buffer) {
    LOG("Failed to retrieve the frame buffer\n");
    return -EFAULT;
  }
  ImageInfo &info = ib->GetImageInfo();
  info.pix_fmt = ConvertToPixFmt(mpp_frame_get_fmt(frame));
  info.width = mpp_frame_get_width(frame);
  info.height = mpp_frame_get_height(frame);
  info.vir_width = mpp_frame_get_hor_stride(frame);
  info.vir_height = mpp_frame_get_ver_stride(frame);
  size_t size = CalPixFmtSize(info.pix_fmt, info.vir_width, info.vir_height);
  auto pts = mpp_frame_get_pts(frame);
  bool eos = mpp_frame_get_eos(frame) ? true : false;
  if (!ib->IsValid()) {
    ib->SetFD(mpp_buffer_get_fd(buffer));
    ib->SetPtr(mpp_buffer_get_ptr(buffer));
    ib->SetSize(mpp_buffer_get_size(buffer));
    ib->SetUserData(frame, __mpp_frame_deinit);
  } else {
    assert(ib->GetSize() >= size);
    if (!ib->IsHwBuffer()) {
      void *ptr = ib->GetPtr();
      assert(ptr);
      LOG("extra time-consuming memcpy to cpu!\n");
      memcpy(ptr, mpp_buffer_get_ptr(buffer), size);
      // sync to cpu?
    }
    mpp_frame_deinit(&frame);
  }
  ib->SetValidSize(size);
  ib->SetTimeStamp(pts);
  ib->SetEOF(eos);

  return 0;
}

int MPPDecoder::Process(std::shared_ptr<MediaBuffer> input,
                        std::shared_ptr<MediaBuffer> output,
                        std::shared_ptr<MediaBuffer> extra_output _UNUSED) {
  if (!support_sync)
    return -ENOSYS;

  if (!input || !input->IsValid())
    return 0;
  if (!output)
    return -EINVAL;
  if (output->GetType() != Type::Image) {
    LOG("mpp decoder output must be image buffer\n");
    return -EINVAL;
  }

  MPP_RET ret;
  MppPacket packet = NULL;
  MppBuffer mpp_buf = NULL;
  MppFrame frame = NULL;
  MppTask task = NULL;
  MppFrame frame_out = NULL;

  ret = init_mpp_buffer_with_content(mpp_buf, input);
  if (ret) {
    LOG("Failed to init MPP buffer with content (ret = %d)\n", ret);
    return -EFAULT;
  }
  assert(mpp_buf);
  ret = mpp_packet_init_with_buffer(&packet, mpp_buf);
  mpp_buffer_put(mpp_buf);
  mpp_buf = NULL;
  if (ret != MPP_OK) {
    LOG("Failed to init MPP packet with buffer (ret = %d)\n", ret);
    goto out;
  }
  assert(packet);
  if (input->IsEOF()) {
    LOG("send eos packet to MPP\n");
    mpp_packet_set_eos(packet);
  }
  if (output->GetSize() == 0) {
    LOG("TODO: mpp need external output buffer now\n");
    goto out;
  }
  ret = init_mpp_buffer(mpp_buf, output, output->GetSize());
  if (ret) {
    LOG("Failed to init MPP buffer (ret = %d)\n", ret);
    goto out;
  }
  if (mpp_buf) {
    ret = mpp_frame_init(&frame);
    if (MPP_OK != ret) {
      LOG("Failed mpp_frame_init (ret = %d)\n", ret);
      goto out;
    }
    mpp_frame_set_buffer(frame, mpp_buf);
    mpp_buffer_put(mpp_buf);
    mpp_buf = NULL;
  }

  ret = mpi->poll(ctx, MPP_PORT_INPUT, MPP_POLL_BLOCK);
  if (ret) {
    LOG("mpp input poll failed (ret = %d)\n", ret);
    goto out;
  }
  ret = mpi->dequeue(ctx, MPP_PORT_INPUT, &task);
  if (ret) {
    LOG("mpp task input dequeue failed (ret = %d)\n", ret);
    goto out;
  }
  mpp_assert(task);
  mpp_task_meta_set_packet(task, KEY_INPUT_PACKET, packet);
  if (frame)
    mpp_task_meta_set_frame(task, KEY_OUTPUT_FRAME, frame);
  ret = mpi->enqueue(ctx, MPP_PORT_INPUT, task);
  if (ret) {
    LOG("mpp task input enqueue failed (ret = %d)\n", ret);
    goto out;
  }
  ret = mpi->poll(ctx, MPP_PORT_OUTPUT, MPP_POLL_BLOCK);
  if (ret) {
    LOG("mpp output poll failed (ret = %d)\n", ret);
    goto out;
  }
  ret = mpi->dequeue(ctx, MPP_PORT_OUTPUT, &task);
  if (ret) {
    LOG("mpp task output dequeue failed (ret = %d)\n", ret);
    goto out;
  }
  mpp_packet_deinit(&packet);
  if (!task)
    goto out;
  mpp_task_meta_get_frame(task, KEY_OUTPUT_FRAME, &frame_out);
  mpp_assert(frame_out == frame); // one in, one out
  if (mpp_frame_get_errinfo(frame_out))
    goto out;
  ret = mpi->enqueue(ctx, MPP_PORT_OUTPUT, task);
  if (ret)
    LOG("mpp task output enqueue failed\n");
  if (SetImageBufferWithMppFrame(std::static_pointer_cast<ImageBuffer>(output),
                                 frame))
    goto out;

  return 0;

out:
  if (mpp_buf)
    mpp_buffer_put(mpp_buf);
  if (packet)
    mpp_packet_deinit(&packet);
  if (frame)
    mpp_frame_deinit(&frame);
  return -EFAULT;
}

int MPPDecoder::SendInput(std::shared_ptr<MediaBuffer> input _UNUSED) {
  if (!input)
    return 0;
  int fret = 0;
  MPP_RET ret;
  MppPacket packet = NULL;
  void *buffer = input->GetPtr();
  size_t size = input->GetValidSize();

  ret = mpp_packet_init(&packet, buffer, size);
  if (ret != MPP_OK) {
    LOG("Failed to init MPP packet (ret = %d)\n", ret);
    return -EFAULT;
  }
  mpp_packet_set_pts(packet, input->GetTimeStamp());
  if (input->IsEOF()) {
    LOG("send eos packet to MPP\n");
    mpp_packet_set_eos(packet);
  }
  ret = mpi->decode_put_packet(ctx, packet);
  if (ret != MPP_OK) {
    if (ret == MPP_ERR_BUFFER_FULL) {
      // LOG("Buffer full writing %d bytes to decoder\n", size);
      fret = -EAGAIN;
    } else {
      LOG("Failed to put a packet to MPP (ret = %d)\n", ret);
      fret = -EFAULT;
    }
  }
  mpp_packet_deinit(&packet);

  return fret;
}

std::shared_ptr<MediaBuffer> MPPDecoder::FetchOutput() {
  MppFrame mppframe = NULL;
  errno = 0;
  MPP_RET ret = mpi->decode_get_frame(ctx, &mppframe);
  if (ret != MPP_OK) {
    // MPP_ERR_TIMEOUT
    LOG("Failed to get a frame from MPP (ret = %d)\n", ret);
    return nullptr;
  }
  if (!mppframe) {
    // LOG("mppframe is NULL\n", mppframe);
    return nullptr;
  }
  if (mpp_frame_get_info_change(mppframe)) {
    MediaConfig &cfg = GetConfig();
    ImageInfo &img_info = cfg.img_cfg.image_info;
    img_info.pix_fmt = ConvertToPixFmt(mpp_frame_get_fmt(mppframe));
    img_info.width = mpp_frame_get_width(mppframe);
    img_info.height = mpp_frame_get_height(mppframe);
    img_info.vir_width = mpp_frame_get_hor_stride(mppframe);
    img_info.vir_height = mpp_frame_get_ver_stride(mppframe);
    ret = mpi->control(ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
    if (ret != MPP_OK)
      LOG("info change ready failed ret = %d\n", ret);
    LOG("MppDec Info change get, %dx%d in (%dx%d)\n", img_info.width,
        img_info.height, img_info.vir_width, img_info.vir_height);
    goto out;
  } else if (mpp_frame_get_eos(mppframe)) {
    LOG("Received EOS frame.\n");
    auto mb = std::make_shared<ImageBuffer>();
    if (!mb) {
      errno = ENOMEM;
      goto out;
    }
    mb->SetTimeStamp(mpp_frame_get_pts(mppframe));
    mb->SetEOF(true);
    mpp_frame_deinit(&mppframe);
    return mb;
  } else if (mpp_frame_get_discard(mppframe)) {
    LOG("Received a discard frame.\n");
    goto out;
  } else if (mpp_frame_get_errinfo(mppframe)) {
    LOG("Received a errinfo frame.\n");
    goto out;
  } else {
    auto mb = std::make_shared<ImageBuffer>();
    if (!mb) {
      errno = ENOMEM;
      goto out;
    }
    if (SetImageBufferWithMppFrame(mb, mppframe))
      goto out;

    return mb;
  }

out:
  mpp_frame_deinit(&mppframe);
  return nullptr;
}

DEFINE_VIDEO_DECODER_FACTORY(MPPDecoder)
const char *FACTORY(MPPDecoder)::ExpectedInputDataType() {
  return TYPENEAR(IMAGE_JPEG) TYPENEAR(VIDEO_H264);
}
const char *FACTORY(MPPDecoder)::OutPutDataType() { return IMAGE_NV12; }

} // namespace easymedia
