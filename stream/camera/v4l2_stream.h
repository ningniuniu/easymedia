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

#ifndef RKMEDIA_V4L2_STREAM_H_
#define RKMEDIA_V4L2_STREAM_H_

#include "stream.h"

#include <assert.h>

#include "v4l2_utils.h"

namespace easymedia {

class V4L2Stream : public Stream {
public:
  V4L2Stream() : use_libv4l2(false), fd(-1) { memset(&vio, 0, sizeof(vio)); }
  virtual ~V4L2Stream() {
    if (fd >= 0)
      close(fd);
  }
  virtual size_t Read(void *ptr _UNUSED, size_t size _UNUSED,
                      size_t nmemb _UNUSED) final {
    return 0;
  }
  virtual int Seek(int64_t offset _UNUSED, int whence _UNUSED) final {
    return -1;
  }
  virtual long Tell() final { return -1; }
  virtual size_t Write(const void *ptr _UNUSED, size_t size _UNUSED,
                       size_t nmemb _UNUSED) final {
    return 0;
  }
#define v4l2_open vio.open_f
#define v4l2_close vio.close_f
#define v4l2_dup vio.dup_f
#define v4l2_ioctl vio.ioctl_f
#define v4l2_read vio.read_f
#define v4l2_mmap vio.mmap_f
#define v4l2_munmap vio.munmap_f
  int IoCtrl(unsigned long int request, ...) override {
    va_list vl;
    va_start(vl, request);
    int ret = V4L2IoCtl(&vio, fd, request, vl);
    va_end(vl);
    return ret;
  }

protected:
  bool use_libv4l2;
  v4l2_io vio;
  std::string device;
  std::string sub_device;
  int fd;
  // static sub_device_controller;
};

} // namespace easymedia

#endif // #ifndef RKMEDIA_V4L2_STREAM_H_
