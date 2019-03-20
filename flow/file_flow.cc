/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "buffer.h"
#include "flow.h"
#include "stream.h"
#include "utils.h"

namespace rkmedia {

class FileReadFlow : public Flow {
public:
  FileReadFlow(const char *param);
  virtual ~FileReadFlow();
  static const char *GetFlowName() { return "file_read_flow"; }

private:
  void ReadThreadRun();

  std::shared_ptr<Stream> fstream;
  std::string path;
  MediaBuffer::MemType mtype;
  size_t read_size;
  ImageInfo info;
  int fps;
  int loop_time;
  bool loop;
  std::thread *read_thread;
};

FileReadFlow::FileReadFlow(const char *param)
    : mtype(MediaBuffer::MemType::MEM_COMMON), read_size(0), fps(0),
      loop_time(0), loop(false), read_thread(nullptr) {
  memset(&info, 0, sizeof(info));
  info.pix_fmt = PIX_FMT_NONE;
  if (!SetAsSource()) {
    errno = EINVAL;
    return;
  }
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params)) {
    errno = EINVAL;
    return;
  }
  std::string s;
  std::string value;
  CHECK_EMPTY_SETERRNO(value, params, KEY_PATH, EINVAL)
  path = value;
  CHECK_EMPTY_SETERRNO(value, params, KEY_OPEN_MODE, EINVAL)
  PARAM_STRING_APPEND(s, KEY_PATH, path);
  PARAM_STRING_APPEND(s, KEY_OPEN_MODE, value);
  fstream = REFLECTOR(Stream)::Create<Stream>("file_read_stream", s.c_str());
  if (!fstream) {
    fprintf(stderr, "Create stream file_read_stream failed\n");
    errno = EINVAL;
    return;
  }
  value = params[KEY_MEM_TYPE];
  if (!value.empty())
    mtype = StringToMemType(value.c_str());
  value = params[KEY_MEM_SIZE_PERTIME];
  if (value.empty()) {
    if (!ParseImageInfoFromMap(params, info)) {
      errno = EINVAL;
      return;
    }
  } else {
    read_size = std::stoul(value);
  }
  value = params[KEY_FPS];
  if (!value.empty())
    fps = std::stoi(value);
  value = params[KEY_LOOP_TIME];
  if (!value.empty())
    loop_time = std::stoi(value);
  SlotMap sm;
  sm.input_slots.push_back(0);
  sm.output_slots.push_back(0);
  sm.process = void_transaction00;
  sm.thread_model = Model::SYNC;
  sm.mode_when_full = InputMode::DROPFRONT;
  if (!InstallSlotMap(sm, path, 0)) {
    LOG("Fail to InstallSlotMap, read %s\n", path.c_str());
    errno = EINVAL;
    return;
  }
  loop = true;
  read_thread = new std::thread(&FileReadFlow::ReadThreadRun, this);
  if (!read_thread) {
    loop = false;
    errno = EINVAL;
    return;
  }
  errno = 0;
}

FileReadFlow::~FileReadFlow() {
  StopAllThread();
  if (read_thread) {
    source_start_cond_mtx->lock();
    loop = false;
    source_start_cond_mtx->notify();
    source_start_cond_mtx->unlock();
    read_thread->join();
    delete read_thread;
  }
  if (fstream)
    fstream->Close();
}

void FileReadFlow::ReadThreadRun() {
  source_start_cond_mtx->lock();
  if (down_flow_num == 0)
    source_start_cond_mtx->wait();
  source_start_cond_mtx->unlock();
  AutoPrintLine apl(__func__);
  size_t alloc_size = read_size;
  bool is_image = (info.pix_fmt != PIX_FMT_NONE);
  if (alloc_size == 0) {
    if (is_image) {
      int num = 0, den = 0;
      GetPixFmtNumDen(info.pix_fmt, num, den);
      alloc_size = info.vir_width * info.vir_height * num * den;
    }
  }
  while (loop) {
    if (fstream->Eof()) {
      if (loop_time-- > 0)
        fstream->Seek(0, SEEK_SET);
      else
        break;
    }
    auto buffer = MediaBuffer::Alloc(alloc_size, mtype);
    if (!buffer) {
      LOG_NO_MEMORY();
      continue;
    }
    if (is_image) {
      auto imagebuffer = std::make_shared<ImageBuffer>(*(buffer.get()), info);
      if (!imagebuffer) {
        LOG_NO_MEMORY();
        continue;
      }
      buffer = imagebuffer;
    }
    size_t size;
    if (read_size) {
      size = fstream->Read(buffer->GetPtr(), 1, read_size);
      if (size != read_size && !fstream->Eof()) {
        LOG("read get %d != expect %d\n", size, read_size);
        SetDisable();
        break;
      }
      buffer->SetValidSize(size);
    }
    if (is_image) {
      if (!fstream->ReadImage(buffer->GetPtr(), info) && !fstream->Eof()) {
        SetDisable();
        break;
      }
      buffer->SetValidSize(buffer->GetSize());
    }
    buffer->SetTimeStamp(gettimeofday());
    SendInput(buffer, 0);
    if (fps != 0) {
      static int interval = 1000 / fps;
      msleep(interval);
    }
  }
}

DEFINE_FLOW_FACTORY(FileReadFlow, Flow)
const char *FACTORY(FileReadFlow)::ExpectedInputDataType() { return nullptr; }
const char *FACTORY(FileReadFlow)::OutPutDataType() { return ""; }

} // namespace rkmedia
