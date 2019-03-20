/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "stream.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>

#include "media_type.h"
#include "utils.h"

namespace rkmedia {

#define CHECK_FILE(f)                                                          \
  if (!f) {                                                                    \
    errno = EBADF;                                                             \
    return -1;                                                                 \
  }

class FileStream : public Stream {
public:
  FileStream(const char *param);
  virtual ~FileStream() { assert(!file); }
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

  virtual bool Eof() final {
    if (!file) {
      errno = EBADF;
      return true;
    }
    return eof ? eof : !!feof(file);
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
std::shared_ptr<Stream>
FACTORY(FileWriteStream)::NewProduct(const char *param) {
  std::shared_ptr<Stream> ret = std::make_shared<FileWriteStream>(param);
  if (ret && ret->Open() < 0)
    return nullptr;
  return ret;
}

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
std::shared_ptr<Stream> FACTORY(FileReadStream)::NewProduct(const char *param) {
  std::shared_ptr<Stream> ret = std::make_shared<FileReadStream>(param);
  if (ret && ret->Open() < 0)
    return nullptr;
  return ret;
}

const char *FACTORY(FileReadStream)::ExpectedInputDataType() {
  return STREAM_FILE;
}

const char *FACTORY(FileReadStream)::OutPutDataType() { return TYPE_ANYTHING; }

} // namespace rkmedia
