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

#include "v4l2_stream.h"

#include <fcntl.h>

#include "control.h"

namespace easymedia {

V4L2Context::V4L2Context(enum v4l2_buf_type cap_type, v4l2_io io_func,
                         const std::string &device)
    : fd(-1), capture_type(cap_type), vio(io_func), started(false)
#ifndef NDEBUG
      ,
      path(device)
#endif
{
  const char *dev = device.c_str();
  fd = v4l2_open(dev, O_RDWR | O_CLOEXEC, 0);
  if (fd < 0)
    LOG("open %s failed %m\n", dev);
  LOGD("open %s, fd %d\n", dev, fd);
}

V4L2Context::~V4L2Context() {
  if (fd >= 0) {
    SetStarted(false);
    v4l2_close(fd);
    LOGD("close %s, fd %d\n", path.c_str(), fd);
  }
}

bool V4L2Context::SetStarted(bool val) {
  std::lock_guard<std::mutex> _lk(mtx);
  if (started == val)
    return true;
  enum v4l2_buf_type cap_type = capture_type;
  unsigned int request = val ? VIDIOC_STREAMON : VIDIOC_STREAMOFF;
  if (IoCtrl(request, &cap_type) < 0) {
    LOG("ioctl(%d): %m\n", (int)request);
    return false;
  }
  started = val;
  return true;
}

int V4L2Context::IoCtrl(unsigned long int request, void *arg) {
  if (fd < 0) {
    errno = EINVAL;
    return -1;
  }
  return V4L2IoCtl(&vio, fd, request, arg);
}

V4L2Stream::V4L2Stream(const char *param)
    : use_libv4l2(false), fd(-1), capture_type(V4L2_BUF_TYPE_VIDEO_CAPTURE) {
  memset(&vio, 0, sizeof(vio));
  std::map<std::string, std::string> params;
  std::list<std::pair<const std::string, std::string &>> req_list;
  std::string str_libv4l2;
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_USE_LIBV4L2, str_libv4l2));
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_DEVICE, device));
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_SUB_DEVICE, sub_device));
  std::string cap_type;
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_V4L2_CAP_TYPE, cap_type));
  int ret = parse_media_param_match(param, params, req_list);
  if (ret == 0)
    return;
  if (!str_libv4l2.empty())
    use_libv4l2 = !!std::stoi(str_libv4l2);
  if (!cap_type.empty())
    capture_type =
        static_cast<enum v4l2_buf_type>(GetV4L2Type(cap_type.c_str()));
}

int V4L2Stream::Open() {
  if (!SetV4L2IoFunction(&vio, use_libv4l2))
    return -EINVAL;
  if (!sub_device.empty()) {
    // TODO:
  }
  v4l2_ctx = std::make_shared<V4L2Context>(capture_type, vio, device);
  if (!v4l2_ctx)
    return -ENOMEM;
  fd = v4l2_ctx->GetDeviceFd();
  if (fd < 0) {
    v4l2_ctx = nullptr;
    return -1;
  }
  return 0;
}

int V4L2Stream::Close() {
  if (v4l2_ctx) {
    v4l2_ctx->SetStarted(false);
    v4l2_ctx = nullptr; // release reference
  }
  fd = -1;
  return 0;
}

int V4L2Stream::IoCtrl(unsigned long int request, ...) {
  va_list vl;
  va_start(vl, request);
  void *arg = va_arg(vl, void *);
  va_end(vl);
  switch (request) {
  case S_SUB_REQUEST: {
    auto sub = (SubRequest *)arg;
    return V4L2IoCtl(&vio, fd, sub->sub_request, sub->arg);
  }
  case S_STREAM_OFF: {
    return v4l2_ctx->SetStarted(false) ? 0 : -1;
  }
  }
  return -1;
}

} // namespace easymedia