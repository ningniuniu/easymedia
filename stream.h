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

#ifndef EASYMEDIA_STREAM_H_
#define EASYMEDIA_STREAM_H_

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

#include "control.h"
#include "image.h"
#include "media_reflector.h"

namespace easymedia {

DECLARE_FACTORY(Stream)

// usage: REFLECTOR(Stream)::Create<T>(streamname, param)
//        T must be the final class type exposed to user
DECLARE_REFLECTOR(Stream)

#define DEFINE_STREAM_FACTORY(REAL_PRODUCT, FINAL_EXPOSE_PRODUCT)              \
  DEFINE_MEDIA_CHILD_FACTORY(REAL_PRODUCT, REAL_PRODUCT::GetStreamName(),      \
                             FINAL_EXPOSE_PRODUCT, Stream)                     \
  DEFINE_MEDIA_CHILD_FACTORY_EXTRA(REAL_PRODUCT)                               \
  DEFINE_MEDIA_NEW_PRODUCT_BY(REAL_PRODUCT, FINAL_EXPOSE_PRODUCT, Open() < 0)

class MediaBuffer;
#ifdef IN_EASYMEDIA_STREAM_CC
static int local_close(void *stream);
#endif
// interface
class _API Stream {
public:
  static StreamOperation c_operations;

  Stream() : readable(false), writeable(false), seekable(false) {}
  virtual ~Stream() = default;

  virtual size_t Read(void *ptr, size_t size, size_t nmemb) = 0;
  virtual size_t Write(const void *ptr, size_t size, size_t nmemb) = 0;
  // whence: SEEK_SET, SEEK_CUR, SEEK_END
  virtual int Seek(int64_t offset, int whence) = 0;
  virtual long Tell() = 0;

  virtual bool Readable() { return readable; }
  virtual bool Writeable() { return writeable; }
  virtual bool Seekable() { return seekable; }

  void SetReadable(bool able) { readable = able; }
  void SetWriteable(bool able) { writeable = able; }
  void SetSeekable(bool able) { seekable = able; }

  virtual bool Eof() { return false; }

  // No need size input. For some device, such as V4L2, always return specific
  // buffer
  virtual std::shared_ptr<MediaBuffer> Read() { return nullptr; }
  virtual bool Write(std::shared_ptr<MediaBuffer>) { return false; }
  // The IoCtrl must be called in the same thread of Read()/Write()
  virtual int IoCtrl(unsigned long int request _UNUSED, ...) { return -1; }
  virtual int SubIoCtrl(unsigned long int request _UNUSED, void *arg) {
    SubRequest subreq = {request, arg};
    return IoCtrl(S_SUB_REQUEST, &subreq);
  }

  // read data as image by ImageInfo
  bool ReadImage(void *ptr, const ImageInfo &info);

protected:
  virtual int Open() = 0;
  virtual int Close() = 0;

  friend int local_close(void *stream);

private:
  bool readable;
  bool writeable;
  bool seekable;

  DECLARE_PART_FINAL_EXPOSE_PRODUCT(Stream)
};

} // namespace easymedia

#endif // #ifndef EASYMEDIA_STREAM_H_
