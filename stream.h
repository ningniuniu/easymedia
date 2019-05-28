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
  DEFINE_MEDIA_CHILD_FACTORY_EXTRA(REAL_PRODUCT)

// interface
class Stream {
public:
  static StreamOperation c_operations;

  Stream() : readable(false), writeable(false), seekable(false) {}
  virtual ~Stream() = default;

  virtual size_t Read(void *ptr, size_t size, size_t nmemb) = 0;
  virtual size_t Write(const void *ptr, size_t size, size_t nmemb) = 0;
  // whence: SEEK_SET, SEEK_CUR, SEEK_END
  virtual int Seek(int64_t offset, int whence) = 0;
  virtual long Tell() = 0;
  virtual int Open() = 0;
  virtual int Close() = 0;

  virtual bool Readable() { return readable; }
  virtual bool Writeable() { return writeable; }
  virtual bool Seekable() { return seekable; }

  void SetReadable(bool able) { readable = able; }
  void SetWriteable(bool able) { writeable = able; }
  void SetSeekable(bool able) { seekable = able; }

  virtual bool Eof() { return false; }

  bool ReadImage(void *ptr, const ImageInfo &info);

private:
  bool readable;
  bool writeable;
  bool seekable;

  DECLARE_PART_FINAL_EXPOSE_PRODUCT(Stream)
};

} // namespace easymedia

#endif // #ifndef RKMEDIA_STREAM_H_
