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

Stream::~Stream() {
  if (priv_resource)
    assert(0 && "fogot release stream resource?");
}

int Stream::Close() {
  priv_resource = nullptr;
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

} // namespace rkmedia
