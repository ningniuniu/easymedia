/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_STREAM_H_
#define RKMEDIA_STREAM_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {

typedef struct {
  int (*close)(void *stream);
  size_t (*read)(void *ptr, size_t size, size_t nmemb, void *stream);
  size_t (*write)(const void *ptr, size_t size, size_t nmemb, void *stream);
  int (*seek)(void *stream, int64_t offset, int whence);
  long (*tell)(void *stream);
} StreamOperation;
}
#endif

#include <algorithm>

namespace rkmedia {

// interface
class Stream {
public:
  static StreamOperation c_operations;

  Stream()
      : priv_resource(nullptr), readable(true), writeable(true),
        seekable(true) {}
  virtual ~Stream();

  virtual size_t Read(void *ptr, size_t size, size_t nmemb) = 0;
  virtual size_t Write(const void *ptr, size_t size, size_t nmemb) = 0;
  // whence: SEEK_SET, SEEK_CUR, SEEK_END
  virtual int Seek(int64_t offset, int whence) = 0;
  virtual long Tell() = 0;
  virtual int Close();

  virtual bool Readable() { return readable; }
  virtual bool Writeable() { return writeable; }
  virtual bool Seekable() { return seekable; }

  void SetReadable(bool able) { readable = able; }
  void SetWriteable(bool able) { writeable = able; }
  void SetSeekable(bool able) { seekable = able; }

protected:
  void *priv_resource;

private:
  bool readable;
  bool writeable;
  bool seekable;
};

} // namespace rkmedia

#endif // #ifndef RKMEDIA_STREAM_H_
