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

#include "stream.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>

#include "media_type.h"
#include "utils.h"

namespace easymedia {

#define CHECK_FILE(f)                                                          \
  if (!f) {                                                                    \
    errno = EBADF;                                                             \
    return -1;                                                                 \
  }

class FileStream : public Stream {
public:
  FileStream(const char *param);
  virtual ~FileStream() {
    if (file)
      FileStream::Close();
  }
  virtual size_t Read(void *ptr, size_t size, size_t nmemb) final {
    if (!Readable())
      return -1;
    CHECK_FILE(file)
    return fread(ptr, size, nmemb, file);
  }
  virtual int Seek(int64_t offset, int whence) final {
    if (!Seekable())
      return -1;
    CHECK_FILE(file)
    return fseek(file, offset, whence);
  }
  virtual long Tell() final {
    CHECK_FILE(file)
    return ftell(file);
  }
  virtual size_t Write(const void *ptr, size_t size, size_t nmemb) final {
    if (!Writeable())
      return -1;
    CHECK_FILE(file)
    return fwrite(ptr, size, nmemb, file);
  }

  virtual bool Eof() final {
    if (!file) {
      errno = EBADF;
      return true;
    }
    return eof ? eof : !!feof(file);
  }

protected:
  virtual int Open() {
    if (path.empty() || open_mode.empty())
      return -1;
    file = fopen(path.c_str(), open_mode.c_str());
    if (!file)
      return -1;
    eof = false;
    SetReadable(true);
    SetWriteable(true);
    SetSeekable(true);
    return 0;
  }
  virtual int Close() final {
    if (!file) {
      errno = EBADF;
      return EOF;
    }
    eof = true;
    int ret = fclose(file);
    file = NULL;
    return ret;
  }

private:
  std::string path;
  std::string open_mode;
  FILE *file;
  bool eof;
};

FileStream::FileStream(const char *param) : file(NULL), eof(true) {
  std::map<std::string, std::string> params;
  std::list<std::pair<const std::string, std::string &>> req_list;
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_PATH, path));
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_OPEN_MODE, open_mode));
  int ret = parse_media_param_match(param, params, req_list);
  UNUSED(ret);
}

// FileWriteStream
class FileWriteStream : public FileStream {
public:
  FileWriteStream(const char *param) : FileStream(param) {}
  virtual ~FileWriteStream() = default;
  static const char *GetStreamName() { return "file_write_stream"; }
  virtual int Open() final {
    int ret = FileStream::Open();
    if (!ret)
      SetReadable(false);
    return ret;
  }
};

DEFINE_STREAM_FACTORY(FileWriteStream, Stream)

const char *FACTORY(FileWriteStream)::ExpectedInputDataType() {
  return TYPE_ANYTHING;
}

const char *FACTORY(FileWriteStream)::OutPutDataType() { return STREAM_FILE; }

// FileReadStream
class FileReadStream : public FileStream {
public:
  FileReadStream(const char *param) : FileStream(param) {}
  virtual ~FileReadStream() = default;
  static const char *GetStreamName() { return "file_read_stream"; }
  virtual int Open() final {
    int ret = FileStream::Open();
    if (!ret)
      SetWriteable(false);
    return ret;
  }
};

DEFINE_STREAM_FACTORY(FileReadStream, Stream)

const char *FACTORY(FileReadStream)::ExpectedInputDataType() {
  return STREAM_FILE;
}

const char *FACTORY(FileReadStream)::OutPutDataType() { return TYPE_ANYTHING; }

} // namespace easymedia
