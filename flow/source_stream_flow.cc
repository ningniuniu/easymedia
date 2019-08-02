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

#include "buffer.h"
#include "flow.h"
#include "stream.h"
#include "utils.h"

namespace easymedia {

class SourceStreamFlow : public Flow {
public:
  SourceStreamFlow(const char *param);
  virtual ~SourceStreamFlow();
  static const char *GetFlowName() { return "source_stream"; }
  virtual int Control(unsigned long int request, ...) final {
    if (!stream)
      return -1;
    va_list vl;
    va_start(vl, request);
    void *arg = va_arg(vl, void *);
    va_end(vl);
    return stream->IoCtrl(request, arg);
  }

private:
  void ReadThreadRun();
  bool loop;
  std::thread *read_thread;
  std::shared_ptr<Stream> stream;
};

SourceStreamFlow::SourceStreamFlow(const char *param)
    : loop(false), read_thread(nullptr) {
  std::list<std::string> separate_list;
  std::map<std::string, std::string> params;
  if (!ParseWrapFlowParams(param, params, separate_list)) {
    SetError(-EINVAL);
    return;
  }
  std::string &name = params[KEY_NAME];
  const char *stream_name = name.c_str();
  const std::string &stream_param = separate_list.back();
  stream = REFLECTOR(Stream)::Create<Stream>(stream_name, stream_param.c_str());
  if (!stream) {
    LOG("Create stream %s failed\n", stream_name);
    SetError(-EINVAL);
    return;
  }
  if (!SetAsSource(std::vector<int>({0}), std::vector<int>({0}),
                   void_transaction00, name)) {
    SetError(-EINVAL);
    return;
  }
  loop = true;
  read_thread = new std::thread(&SourceStreamFlow::ReadThreadRun, this);
  if (!read_thread) {
    loop = false;
    SetError(-EINVAL);
    return;
  }
}

SourceStreamFlow::~SourceStreamFlow() {
  loop = false;
  StopAllThread();
  int stop = 1;
  if (stream && Control(S_STREAM_OFF, &stop))
    LOG("Fail to stop source stream\n");
  if (read_thread) {
    source_start_cond_mtx->lock();
    loop = false;
    source_start_cond_mtx->notify();
    source_start_cond_mtx->unlock();
    read_thread->join();
    delete read_thread;
  }
  stream.reset();
}

void SourceStreamFlow::ReadThreadRun() {
  source_start_cond_mtx->lock();
  if (down_flow_num == 0 && IsEnable())
    source_start_cond_mtx->wait();
  source_start_cond_mtx->unlock();
  while (loop) {
    if (stream->Eof()) {
      // TODO: tell that I reach eof
      SetDisable();
      break;
    }
    auto buffer = stream->Read();
    SendInput(buffer, 0);
  }
}

DEFINE_FLOW_FACTORY(SourceStreamFlow, Flow)
const char *FACTORY(SourceStreamFlow)::ExpectedInputDataType() {
  return nullptr;
}

// TODO!
const char *FACTORY(SourceStreamFlow)::OutPutDataType() { return ""; }

} // namespace easymedia
