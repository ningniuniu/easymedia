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

#include <sys/mman.h>

#include <vector>

#include "buffer.h"
#include "v4l2_stream.h"

namespace easymedia {

class V4L2CaptureStream : public V4L2Stream {
public:
  V4L2CaptureStream(const char *param);
  virtual ~V4L2CaptureStream() { assert(!started); }
  static const char *GetStreamName() { return "v4l2_capture_stream"; }
  virtual std::shared_ptr<MediaBuffer> Read();
  virtual int Open() final;
  virtual int Close() final;

private:
  int BufferExport(enum v4l2_buf_type bt, int index, int *dmafd);

  enum v4l2_memory memory_type;
  std::string data_type;
  int width, height;
  int loop_num;
  std::vector<MediaBuffer> buffer_vec;
  bool started;
};

V4L2CaptureStream::V4L2CaptureStream(const char *param)
    : V4L2Stream(param), memory_type(V4L2_MEMORY_MMAP), data_type(IMAGE_NV12),
      width(0), height(0), loop_num(2), started(false) {
  if (device.empty())
    return;
  std::map<std::string, std::string> params;
  std::list<std::pair<const std::string, std::string &>> req_list;

  std::string mem_type, str_loop_num;
  std::string str_width, str_height;
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_V4L2_MEM_TYPE, mem_type));
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_FRAMES, str_loop_num));
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_OUTPUTDATATYPE, data_type));
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_BUFFER_WIDTH, str_width));
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_BUFFER_HEIGHT, str_height));
  int ret = parse_media_param_match(param, params, req_list);
  if (ret == 0)
    return;
  if (!mem_type.empty())
    memory_type = static_cast<enum v4l2_memory>(GetV4L2Type(mem_type.c_str()));
  if (!str_loop_num.empty())
    loop_num = std::stoi(str_loop_num);
  assert(loop_num >= 2);
  if (!str_width.empty())
    width = std::stoi(str_width);
  if (!str_height.empty())
    height = std::stoi(str_height);
}

int V4L2CaptureStream::BufferExport(enum v4l2_buf_type bt, int index,
                                    int *dmafd) {
  struct v4l2_exportbuffer expbuf;

  memset(&expbuf, 0, sizeof(expbuf));
  expbuf.type = bt;
  expbuf.index = index;
  if (IoCtrl(VIDIOC_EXPBUF, &expbuf) == -1) {
    LOG("VIDIOC_EXPBUF  %d failed, %m", index);
    return -1;
  }
  *dmafd = expbuf.fd;

  return 0;
}

class V4L2Buffer {
public:
  V4L2Buffer() : dmafd(-1), ptr(nullptr), length(0), munmap_f(nullptr) {}
  ~V4L2Buffer() {
    if (dmafd >= 0)
      close(dmafd);
    if (ptr && ptr != MAP_FAILED && munmap_f)
      munmap_f(ptr, length);
  }
  int dmafd;
  void *ptr;
  size_t length;
  int (*munmap_f)(void *_start, size_t length);
};

static int __free_v4l2buffer(void *arg) {
  delete static_cast<V4L2Buffer *>(arg);
  return 0;
}

int V4L2CaptureStream::Open() {
  const char *dev = device.c_str();
  if (width <= 0 || height <= 0) {
    LOG("Invalid param, device=%s, width=%d, height=%d\n", dev, width, height);
    return -EINVAL;
  }
  int ret = V4L2Stream::Open();
  if (ret)
    return ret;

  struct v4l2_capability cap;
  memset(&cap, 0, sizeof(cap));
  if (IoCtrl(VIDIOC_QUERYCAP, &cap) < 0) {
    LOG("Failed to ioctl(VIDIOC_QUERYCAP): %m\n");
    return -1;
  }
  if ((capture_type == V4L2_BUF_TYPE_VIDEO_CAPTURE) &&
      !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    LOG("%s, Not a video capture device.\n", dev);
    return -1;
  }
  if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
    LOG("%s does not support the streaming I/O method.\n", dev);
    return -1;
  }
  const char *data_type_str = data_type.c_str();
  struct v4l2_format fmt;
  fmt.type = capture_type;
  fmt.fmt.pix.width = width;
  fmt.fmt.pix.height = height;
  fmt.fmt.pix.pixelformat = GetV4L2FmtByString(data_type_str);
  fmt.fmt.pix.field = V4L2_FIELD_ANY;
  if (fmt.fmt.pix.pixelformat == 0) {
    LOG("unsupport input format : %s\n", data_type_str);
    return -1;
  }
  if (IoCtrl(VIDIOC_S_FMT, &fmt) < 0) {
    LOG("%s, s fmt failed(cap type=%d, %c%c%c%c), %m\n", dev, capture_type,
        DUMP_FOURCC(fmt.fmt.pix.pixelformat));
    return -1;
  }
  if (GetV4L2FmtByString(data_type_str) != fmt.fmt.pix.pixelformat) {
    LOG("%s, expect %s, return %c%c%c%c\n", dev, data_type_str,
        DUMP_FOURCC(fmt.fmt.pix.pixelformat));
    return -1;
  }
  if (width != (int)fmt.fmt.pix.width || height != (int)fmt.fmt.pix.height) {
    LOG("%s change res from %dx%d to %dx%d\n", dev, width, height,
        fmt.fmt.pix.width, fmt.fmt.pix.height);
    width = fmt.fmt.pix.width;
    height = fmt.fmt.pix.height;
  }
  if (fmt.fmt.pix.field == V4L2_FIELD_INTERLACED)
    LOG("%s is using the interlaced mode\n", dev);

  struct v4l2_requestbuffers req;
  req.type = capture_type;
  req.count = loop_num;
  req.memory = memory_type;
  if (IoCtrl(VIDIOC_REQBUFS, &req) < 0) {
    LOG("%s, count=%d, ioctl(VIDIOC_REQBUFS): %m\n", dev, loop_num);
    return -1;
  }
  int w = UPALIGNTO16(width);
  int h = UPALIGNTO16(height);
  if (memory_type == V4L2_MEMORY_DMABUF) {
    PixelFormat pix_fmt = GetPixFmtByString(data_type_str);
    int size =
        (pix_fmt != PIX_FMT_NONE) ? CalPixFmtSize(pix_fmt, w, h) : w * h * 4;
    for (size_t i = 0; i < req.count; i++) {
      struct v4l2_buffer buf;
      memset(&buf, 0, sizeof(buf));
      buf.type = req.type;
      buf.index = i;
      buf.memory = req.memory;

      auto &&buffer =
          MediaBuffer::Alloc2(size, MediaBuffer::MemType::MEM_HARD_WARE);
      if (buffer.GetSize() == 0) {
        errno = ENOMEM;
        return -1;
      }
      buffer_vec.push_back(buffer);
      buf.m.fd = buffer.GetFD();
      buf.length = buffer.GetSize();
      if (IoCtrl(VIDIOC_QBUF, &buf) < 0) {
        LOG("%s ioctl(VIDIOC_QBUF): %m\n", dev);
        return -1;
      }
    }
  } else if (memory_type == V4L2_MEMORY_MMAP) {
    for (size_t i = 0; i < req.count; i++) {
      struct v4l2_buffer buf;
      void *ptr = MAP_FAILED;

      V4L2Buffer *buffer = new V4L2Buffer();
      if (!buffer) {
        errno = ENOMEM;
        return -1;
      }
      buffer_vec.push_back(
          MediaBuffer(nullptr, 0, -1, buffer, __free_v4l2buffer));
      memset(&buf, 0, sizeof(buf));
      buf.type = req.type;
      buf.index = i;
      buf.memory = req.memory;
      if (IoCtrl(VIDIOC_QUERYBUF, &buf) < 0) {
        LOG("%s ioctl(VIDIOC_QUERYBUF): %m\n", dev);
        return -1;
      }
      ptr = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                      buf.m.offset);
      if (ptr == MAP_FAILED) {
        LOG("%s v4l2_mmap (%d): %m\n", dev, (int)i);
        return -1;
      }
      MediaBuffer &mb = buffer_vec[i];
      buffer->munmap_f = vio.munmap_f;
      buffer->ptr = ptr;
      mb.SetPtr(ptr);
      buffer->length = buf.length;
      mb.SetSize(buf.length);
    }
    for (size_t i = 0; i < req.count; ++i) {
      struct v4l2_buffer buf;
      int dmafd = -1;

      memset(&buf, 0, sizeof(buf));
      buf.type = req.type;
      buf.memory = req.memory;
      buf.index = i;
      if (IoCtrl(VIDIOC_QBUF, &buf) < 0) {
        LOG("%s, ioctl(VIDIOC_QBUF): %m\n", dev);
        return -1;
      }
      if (!BufferExport(capture_type, i, &dmafd)) {
        MediaBuffer &mb = buffer_vec[i];
        V4L2Buffer *buffer = static_cast<V4L2Buffer *>(mb.GetUserData().get());
        buffer->dmafd = dmafd;
        mb.SetFD(dmafd);
      }
    }
  }

  SetReadable(true);
  return 0;
}

int V4L2CaptureStream::Close() {
  started = false;
  return V4L2Stream::Close();
}

class AutoQBUFMediaBuffer : public MediaBuffer {
public:
  AutoQBUFMediaBuffer(MediaBuffer &mb, std::shared_ptr<V4L2Context> ctx,
                      struct v4l2_buffer buf)
      : MediaBuffer(mb), v4l2_ctx(ctx), v4l2_buf(buf) {}
  ~AutoQBUFMediaBuffer() {
    if (v4l2_ctx->IoCtrl(VIDIOC_QBUF, &v4l2_buf) < 0)
      LOG("index=%d, ioctl(VIDIOC_QBUF): %m\n", v4l2_buf.index);
  }

private:
  std::shared_ptr<V4L2Context> v4l2_ctx;
  struct v4l2_buffer v4l2_buf;
};

std::shared_ptr<MediaBuffer> V4L2CaptureStream::Read() {
  const char *dev = device.c_str();
  if (!started && v4l2_ctx->SetStarted(true))
    started = true;
  struct v4l2_buffer buf;
  memset(&buf, 0, sizeof(buf));
  buf.type = capture_type;
  buf.memory = memory_type;
  int ret = IoCtrl(VIDIOC_DQBUF, &buf);
  if (ret < 0) {
    LOG("%s, ioctl(VIDIOC_DQBUF): %m\n", dev);
    return nullptr;
  }
  struct timeval buf_ts = buf.timestamp;
  MediaBuffer &mb = buffer_vec[buf.index];
  auto ret_buf = std::make_shared<AutoQBUFMediaBuffer>(mb, v4l2_ctx, buf);
  if (ret_buf) {
    assert(ret_buf->GetFD() == mb.GetFD());
    if (buf.memory == V4L2_MEMORY_DMABUF) {
      assert(ret_buf->GetFD() == buf.m.fd);
    }
    ret_buf->SetTimeStamp(buf_ts.tv_sec * 1000LL + buf_ts.tv_usec / 1000LL);
    assert(buf.length > 0);
    ret_buf->SetValidSize(buf.length);
  } else {
    if (IoCtrl(VIDIOC_QBUF, &buf) < 0)
      LOG("%s, index=%d, ioctl(VIDIOC_QBUF): %m\n", dev, buf.index);
  }

  return ret_buf;
}

DEFINE_STREAM_FACTORY(V4L2CaptureStream, Stream)

const char *FACTORY(V4L2CaptureStream)::ExpectedInputDataType() {
  return nullptr;
}

const char *FACTORY(V4L2CaptureStream)::OutPutDataType() {
  static std::string ot;
  ot.append(TYPENEAR(IMAGE_YUV420P));
  ot.append(TYPENEAR(IMAGE_NV12));
  ot.append(TYPENEAR(IMAGE_NV21));
  ot.append(TYPENEAR(IMAGE_YUV422P));
  ot.append(TYPENEAR(IMAGE_NV16));
  ot.append(TYPENEAR(IMAGE_NV61));
  ot.append(TYPENEAR(IMAGE_YUYV422));
  ot.append(TYPENEAR(IMAGE_UYVY422));
  ot.append(TYPENEAR(IMAGE_RGB565));
  ot.append(TYPENEAR(IMAGE_RGB888));
  ot.append(TYPENEAR(IMAGE_BGR888));
  ot.append(TYPENEAR(IMAGE_ARGB8888));
  ot.append(TYPENEAR(IMAGE_ABGR8888));
  ot.append(TYPENEAR(IMAGE_JPEG));
  ot.append(TYPENEAR(VIDEO_H264));
  return ot.c_str();
}

} // namespace easymedia