/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "stream.h"

#include <assert.h>
#include <errno.h>

#include "utils.h"

namespace rkmedia {

static size_t local_read(void *ptr, size_t size, size_t nmemb, void *stream) {
  Stream *s = static_cast<Stream *>(stream);
  return s->Read(ptr, size, nmemb);
}

static size_t local_write(const void *ptr, size_t size, size_t nmemb,
                          void *stream) {
  Stream *s = static_cast<Stream *>(stream);
  return s->Write(ptr, size, nmemb);
}

static int local_seek(void *stream, int64_t offset, int whence) {
  Stream *s = static_cast<Stream *>(stream);
  return s->Seek(offset, whence);
}

static long local_tell(void *stream) {
  Stream *s = static_cast<Stream *>(stream);
  return s->Tell();
}

static int local_close(void *stream) {
  Stream *s = static_cast<Stream *>(stream);
  return s->Close();
}

StreamOperation Stream::c_operations = {
  close : local_close,
  read : local_read,
  write : local_write,
  seek : local_seek,
  tell : local_tell
};

int Stream::Close() {
  readable = false;
  writeable = false;
  seekable = false;
  return 0;
}

DEFINE_REFLECTOR(Stream)

// define the base factory missing definition
const char *FACTORY(Stream)::Parse(const char *request) {
  // request should equal stream_name
  return request;
}

DEFINE_PART_FINAL_EXPOSE_PRODUCT(Stream, Stream)

bool Stream::ReadImage(void *ptr, const ImageInfo &info) {
  size_t read_size;
  if (info.width == info.vir_width && info.height == info.vir_height) {
    int num, den;
    GetPixFmtNumDen(info.pix_fmt, num, den);
    read_size = info.width * info.height * num / den;
    if (!read_size)
      return false;
    size_t ret = Read(ptr, 1, read_size);
    if (ret != read_size) {
      // LOG("read ori stream failed, %d != %d\n", ret, read_size);
      return false;
    }
    return true;
  }
  int row = 0;
  uint8_t *buf_y = (uint8_t *)ptr;
  uint8_t *buf_u = buf_y + info.vir_width * info.vir_height;
  // uint8_t *buf_v = buf_u + info.vir_width * info.vir_height / 4;
  switch (info.pix_fmt) {
  case PIX_FMT_NV12:
    for (row = 0; row < info.height; row++) {
      read_size = Read(buf_y + row * info.vir_width, 1, info.width);
      if ((int)read_size != info.width) {
        // LOG("read ori yuv stream luma failed, %d != %d\n",
        // read_size,info.width);
        return false;
      }
    }
    for (row = 0; row < info.height / 2; row++) {
      read_size = Read(buf_u + row * info.vir_width, 1, info.width);
      if ((int)read_size != info.width) {
        // LOG("read ori yuv stream cb failed, %d != %d\n", read_size,
        // info.width);
        return false;
      }
    }
    break;
  default:
    LOG("TODO: read image fmt %d\n", info.pix_fmt);
    return false;
  }
  return true;
}

} // namespace rkmedia
