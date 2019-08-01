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

#include "flow.h"
#include "stream.h"

namespace easymedia {

static bool send_buffer(Flow *f, MediaBufferVector &input_vector);

class OutPutStreamFlow : public Flow {
public:
  OutPutStreamFlow(const char *param);
  virtual ~OutPutStreamFlow() { StopAllThread(); };
  static const char *GetFlowName() { return "output_stream"; }
  virtual int Control(unsigned long int request, ...) final {
    if (!out_stream)
      return -1;
    va_list vl;
    va_start(vl, request);
    void *arg = va_arg(vl, void *);
    va_end(vl);
    return out_stream->IoCtrl(request, arg);
  }

private:
  std::shared_ptr<Stream> out_stream;
  friend bool send_buffer(Flow *f, MediaBufferVector &input_vector);
};

OutPutStreamFlow::OutPutStreamFlow(const char *param) {
  std::list<std::string> separate_list;
  std::map<std::string, std::string> params;
  if (!ParseWrapFlowParams(param, params, separate_list)) {
    SetError(-EINVAL);
    return;
  }
  std::string &name = params[KEY_NAME];
  const char *stream_name = name.c_str();
  SlotMap sm;
  int input_maxcachenum = 1;
  ParseParamToSlotMap(params, sm, input_maxcachenum);
  if (sm.thread_model == Model::NONE)
    sm.thread_model =
        !params[KEY_FPS].empty() ? Model::ASYNCATOMIC : Model::ASYNCCOMMON;
  if (sm.mode_when_full == InputMode::NONE)
    sm.mode_when_full = InputMode::DROPCURRENT;

  const std::string &stream_param = separate_list.back();
  auto stream =
      REFLECTOR(Stream)::Create<Stream>(stream_name, stream_param.c_str());
  if (!stream) {
    LOG("Fail to create stream %s\n", stream_name);
    SetError(-EINVAL);
    return;
  }
  sm.input_slots.push_back(0);
  sm.input_maxcachenum.push_back(input_maxcachenum);
  // sm.output_slots.push_back(0);
  sm.process = send_buffer;
  if (!InstallSlotMap(sm, name, -1)) {
    LOG("Fail to InstallSlotMap, %s\n", stream_name);
    SetError(-EINVAL);
    return;
  }
  out_stream = stream;
}

bool send_buffer(Flow *f, MediaBufferVector &input_vector) {
  OutPutStreamFlow *flow = static_cast<OutPutStreamFlow *>(f);
  auto &buffer = input_vector[0];
  if (!buffer)
    return true;
  return flow->out_stream->Write(buffer);
}

DEFINE_FLOW_FACTORY(OutPutStreamFlow, Flow)
// TODO!
const char *FACTORY(OutPutStreamFlow)::ExpectedInputDataType() { return ""; }
const char *FACTORY(OutPutStreamFlow)::OutPutDataType() { return nullptr; }

} // namespace easymedia
