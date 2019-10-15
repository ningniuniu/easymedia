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

#include "flow.h"

#include <assert.h>

#include <algorithm>

#include "buffer.h"
#include "key_string.h"
#include "utils.h"

namespace easymedia {

class FlowCoroutine {
public:
  FlowCoroutine(Flow *f, Model sync_model, FunctionProcess func, float inter);
  ~FlowCoroutine();

  void Bind(std::vector<int> &in, std::vector<int> &out);
  bool Start();
  void RunOnce();

private:
  void WhileRun();
  void WhileRunSleep();
  void SyncFetchInput(MediaBufferVector &in);
  void ASyncFetchInputCommon(MediaBufferVector &in);
  void ASyncFetchInputAtomic(MediaBufferVector &in);

  void SendNullBufferDown(Flow::FlowMap &fm, const MediaBufferVector &in,
                          std::list<Flow::FlowInputMap> &flows);
  void SendBufferDown(Flow::FlowMap &fm, const MediaBufferVector &in,
                      std::list<Flow::FlowInputMap> &flows, bool process_ret);
  void SendBufferDownFromDeque(Flow::FlowMap &fm, const MediaBufferVector &in,
                               std::list<Flow::FlowInputMap> &flows,
                               bool process_ret);
  size_t OutputHoldRelated(Flow::FlowMap &fm,
                           std::shared_ptr<MediaBuffer> &out_buffer,
                           const MediaBufferVector &input_vector);

  Flow *flow;
  Model model;
  float interval;
  std::vector<int> in_slots;
  std::vector<int> out_slots;
  std::thread *th;
  FunctionProcess th_run;

  MediaBufferVector in_vector;
  decltype(&FlowCoroutine::SyncFetchInput) fetch_input_func;
  decltype(&FlowCoroutine::SendBufferDown) send_down_func;
#ifndef NDEBUG
public:
  void SetMarkName(std::string s) { name = s; }
  void SetExpectProcessTime(int time) { expect_process_time = time; }

  std::string name;
  int expect_process_time; // ms
#endif
};

FlowCoroutine::FlowCoroutine(Flow *f, Model sync_model, FunctionProcess func,
                             float inter)
    : flow(f), model(sync_model), interval(inter), th(nullptr), th_run(func)
#ifndef NDEBUG
      ,
      expect_process_time(0)
#endif
{
}

FlowCoroutine::~FlowCoroutine() {
  if (th) {
    th->join();
    delete th;
  }
  LOGD("%s quit\n", name.c_str());
}

void FlowCoroutine::Bind(std::vector<int> &in, std::vector<int> &out) {
  assert(in_slots.size() == 0 && out_slots.size() == 0 &&
         "flow coroutine binded");
  in_slots = in;
  out_slots = out;
}

bool FlowCoroutine::Start() {
  bool need_thread = false;
  auto func = &FlowCoroutine::WhileRun;
  switch (model) {
  case Model::ASYNCCOMMON:
    need_thread = true;
    fetch_input_func = &FlowCoroutine::ASyncFetchInputCommon;
    send_down_func = &FlowCoroutine::SendBufferDownFromDeque;
    break;
  case Model::ASYNCATOMIC:
    need_thread = true;
    fetch_input_func = &FlowCoroutine::ASyncFetchInputAtomic;
    send_down_func = &FlowCoroutine::SendBufferDown;
    func = &FlowCoroutine::WhileRunSleep;
    break;
  case Model::SYNC:
    fetch_input_func = &FlowCoroutine::SyncFetchInput;
    send_down_func = &FlowCoroutine::SendBufferDown;
    break;
  default:
    LOG("invalid model %d\n", (int)model);
    return false;
  }
  in_vector.resize(in_slots.size());
  if (need_thread) {
    th = new std::thread(func, this);
    if (!th) {
      errno = ENOMEM;
      return false;
    }
  }
  return true;
}

#ifndef NDEBUG
static void check_consume_time(const char *name, int expect, int exactly) {
  if (exactly > expect) {
    LOG("%s, expect consume %d ms, however %d ms\n", name, expect, exactly);
  }
}
#endif

void FlowCoroutine::RunOnce() {
  bool ret;
  (this->*fetch_input_func)(in_vector);
#ifndef NDEBUG
  {
    AutoDuration ad;
#endif
    ret = (*th_run)(flow, in_vector);
#ifndef NDEBUG
    if (expect_process_time > 0)
      check_consume_time(name.c_str(), expect_process_time,
                         (int)(ad.Get() / 1000));
  }
#endif // DEBUG
  for (int idx : out_slots) {
    auto &fm = flow->downflowmap[idx];
    std::list<Flow::FlowInputMap> flows;
    fm.list_mtx.read_lock();
    flows = fm.flows;
    fm.list_mtx.unlock();
    (this->*send_down_func)(fm, in_vector, flows, ret);
  }
  for (auto &buffer : in_vector)
    buffer.reset();
}

void FlowCoroutine::WhileRun() {
  while (!flow->quit)
    RunOnce();
}

void FlowCoroutine::WhileRunSleep() {
  int64_t times = 0;
  AutoDuration ad;
  assert(interval > 0);
  while (!flow->quit) {
    if (times == 0)
      ad.Reset();
    RunOnce();
    ++times;
    int64_t remain = (interval * times - ad.Get()) / 1000;
    if (remain > 0)
      msleep((int)remain);
    if (times >= 10000)
      times = 0;
  }
}

void FlowCoroutine::SyncFetchInput(MediaBufferVector &in) {
  int i = 0;
  for (int idx : in_slots) {
    auto &buffer = flow->v_input[idx].cached_buffer;
    in[i++] = buffer;
    buffer.reset();
  }
}

void FlowCoroutine::ASyncFetchInputCommon(MediaBufferVector &in) {
  for (size_t i = 0; i < in_slots.size(); i++) {
    int idx = in_slots[i];
    auto &input = flow->v_input[idx];
    AutoLockMutex _am(input.cond_mtx);
    auto &v = input.cached_buffers;
    if (v.empty()) {
      if (!input.fetch_block) {
        in[i] = nullptr;
        continue;
      }
      input.cond_mtx.wait();
    }
    if (!flow->enable) {
      in.assign(in_slots.size(), nullptr);
      break;
    }
    assert(!v.empty());
    in[i] = v.front();
    v.pop_front();
  }
}

void FlowCoroutine::ASyncFetchInputAtomic(MediaBufferVector &in) {
  int i = 0;
  for (int idx : in_slots) {
    std::shared_ptr<MediaBuffer> buffer;
    auto &input = flow->v_input[idx];
    input.spin_mtx.lock();
    buffer = input.cached_buffer;
    input.spin_mtx.unlock();
    in[i++] = buffer;
  }
}

void FlowCoroutine::SendNullBufferDown(Flow::FlowMap &fm,
                                       const MediaBufferVector &in,
                                       std::list<Flow::FlowInputMap> &flows) {
  std::shared_ptr<MediaBuffer> nullbuffer;
  if (fm.hold_input != HoldInputMode::NONE) {
    auto empty_result = std::make_shared<easymedia::MediaBuffer>();
    if (empty_result && OutputHoldRelated(fm, empty_result, in) > 0)
      nullbuffer = empty_result;
  }
  for (auto &f : flows)
    f.flow->SendInput(nullbuffer, f.index_of_in);
}

void FlowCoroutine::SendBufferDown(Flow::FlowMap &fm,
                                   const MediaBufferVector &in,
                                   std::list<Flow::FlowInputMap> &flows,
                                   bool process_ret) {
  if (!process_ret) {
    SendNullBufferDown(fm, in, flows);
    return;
  }
  for (auto &f : flows) {
    OutputHoldRelated(fm, fm.cached_buffer, in);
    f.flow->SendInput(fm.cached_buffer, f.index_of_in);
  }
}

void FlowCoroutine::SendBufferDownFromDeque(
    Flow::FlowMap &fm, const MediaBufferVector &in,
    std::list<Flow::FlowInputMap> &flows, bool process_ret) {
  if (!process_ret) {
    SendNullBufferDown(fm, in, flows);
    return;
  }
  if (fm.cached_buffers.empty())
    return;
  for (auto &buffer : fm.cached_buffers) {
    OutputHoldRelated(fm, buffer, in);
    for (auto &f : flows)
      f.flow->SendInput(buffer, f.index_of_in);
  }
  fm.cached_buffers.clear();
}

size_t
FlowCoroutine::OutputHoldRelated(Flow::FlowMap &fm,
                                 std::shared_ptr<MediaBuffer> &out_buffer,
                                 const MediaBufferVector &input_vector) {
  if (fm.hold_input == HoldInputMode::HOLD_INPUT)
    return FlowOutputHoldInput(out_buffer, input_vector);
  else if (fm.hold_input == HoldInputMode::INHERIT_FORM_INPUT)
    return FlowOutputInheritFromInput(out_buffer, input_vector);
  return 0;
}

DEFINE_REFLECTOR(Flow)
DEFINE_FACTORY_COMMON_PARSE(Flow)
DEFINE_PART_FINAL_EXPOSE_PRODUCT(Flow, Flow)

const FunctionProcess Flow::void_transaction00 = void_transaction<0, 0>;

Flow::Flow()
    : out_slot_num(0), input_slot_num(0), down_flow_num(0), enable(true),
      quit(false) {}

Flow::~Flow() { StopAllThread(); }

void Flow::StopAllThread() {
  for (auto &in : v_input) {
    AutoLockMutex _alm(in.cond_mtx);
    enable = false;
    quit = true;
    in.cond_mtx.notify();
  }
  for (auto &coroutine : coroutines)
    coroutine.reset();
}

static bool check_slots(std::vector<int> &slots, const char *debugstr) {
  if (slots.empty())
    return true;
  std::sort(slots.begin(), slots.end());
  auto iend = std::unique(slots.begin(), slots.end());
  if (iend != slots.end()) {
    LOG("%s slot duplicate :", debugstr);
    while (iend != slots.end()) {
      LOG(" %d", *iend);
      iend++;
    }
    LOG("\n");
    return false;
  }
  return true;
}

Flow::FlowMap::FlowMap(FlowMap &&fm) {
  if (fm.valid) {
    LOG("Flow::FlowMap is not copyable and moveable after inited\n");
    assert(0);
  }
}

void Flow::FlowMap::Init(Model m, HoldInputMode hold_in) {
  assert(!valid);
  valid = true;
  if (m == Model::ASYNCCOMMON)
    set_output_behavior = &FlowMap::SetOutputToQueueBehavior;
  else
    set_output_behavior = &FlowMap::SetOutputBehavior;
  hold_input = hold_in;
}

void Flow::FlowMap::SetOutputBehavior(
    const std::shared_ptr<MediaBuffer> &output) {
  cached_buffer = output;
}
void Flow::FlowMap::SetOutputToQueueBehavior(
    const std::shared_ptr<MediaBuffer> &output) {
  cached_buffers.push_back(output);
}

Flow::Input::Input(Input &&in) {
  if (in.valid) {
    LOG("Flow::Input is not copyable and moveable after inited\n");
    assert(0);
  }
}

void Flow::Input::Init(Flow *f, Model m, int mcn, InputMode im, bool f_block,
                       std::shared_ptr<FlowCoroutine> fc) {
  assert(!valid);
  valid = true;
  flow = f;
  thread_model = m;
  fetch_block = f_block;
  max_cache_num = mcn;
  mode_when_full = im;
  switch (m) {
  case Model::ASYNCCOMMON:
    send_input_behavior = &Input::ASyncSendInputCommonBehavior;
    break;
  case Model::ASYNCATOMIC:
    send_input_behavior = &Input::ASyncSendInputAtomicBehavior;
    break;
  case Model::SYNC:
    send_input_behavior = &Input::SyncSendInputBehavior;
    coroutine = fc;
    break;
  default:
    break;
  }
  switch (im) {
  case InputMode::BLOCKING:
    async_full_behavior = &Input::ASyncFullBlockingBehavior;
    break;
  case InputMode::DROPFRONT:
    async_full_behavior = &Input::ASyncFullDropFrontBehavior;
    break;
  case InputMode::DROPCURRENT:
    async_full_behavior = &Input::ASyncFullDropCurrentBehavior;
    break;
  default:
    break;
  }
}

bool Flow::SetAsSource(const std::vector<int> &input_slots,
                       const std::vector<int> &output_slots, FunctionProcess f,
                       const std::string &mark) {
  source_start_cond_mtx = std::make_shared<ConditionLockMutex>();
  if (!source_start_cond_mtx)
    return false;
  SlotMap sm;
  sm.input_slots = input_slots;
  sm.output_slots = output_slots;
  sm.process = f;
  sm.thread_model = Model::SYNC;
  sm.mode_when_full = InputMode::DROPFRONT;
  if (!InstallSlotMap(sm, mark, 0)) {
    LOG("Fail to InstallSlotMap, read %s\n", mark.c_str());
    return false;
  }
  return true;
}

bool Flow::InstallSlotMap(SlotMap &map, const std::string &mark,
                          int exp_process_time) {
  LOGD("%s, thread_model=%d, mode_when_full=%d\n", mark.c_str(),
       map.thread_model, map.mode_when_full);
  // parameters validity check
  auto &in_slots = map.input_slots;
  if (in_slots.size() > 1 && map.thread_model == Model::SYNC) {
    LOG("More than 1 input to flow, can not set sync input\n");
    return false;
  }
  if (!check_slots(in_slots, "input"))
    return false;
  bool ret = true;
  for (int i : in_slots) {
    if (i >= (int)v_input.size())
      continue;
    if (v_input[i].valid) {
      LOG("input slot %d has been set\n", i);
      ret = false;
    }
  }
  if (!ret)
    return false;

  auto &out_slots = map.output_slots;
  if (!check_slots(out_slots, "output"))
    return false;
  for (int i : out_slots) {
    if (i >= (int)downflowmap.size())
      continue;
    if (downflowmap[i].valid) {
      LOG("output slot %d has been set\n", i);
      ret = false;
    }
  }
  if (!ret)
    return false;

  auto c = std::make_shared<FlowCoroutine>(this, map.thread_model, map.process,
                                           map.interval);
  if (!c) {
    errno = ENOMEM;
    return false;
  }
  c->Bind(in_slots, out_slots);
  coroutines.push_back(c);
  if (!in_slots.empty()) {
    int max_idx = in_slots[in_slots.size() - 1];
    if ((int)v_input.size() <= max_idx)
      v_input.resize(max_idx + 1);
    for (size_t i = 0; i < in_slots.size(); i++) {
      v_input[in_slots[i]].Init(
          this, map.thread_model,
          (map.thread_model == Model::ASYNCCOMMON) ? map.input_maxcachenum[i]
                                                   : 0,
          map.mode_when_full,
          (map.thread_model == Model::ASYNCCOMMON && map.fetch_block.size() > i)
              ? map.fetch_block[i]
              : true,
          c);
      input_slot_num++;
    }
  }
  if (!out_slots.empty()) {
    int max_idx = out_slots[out_slots.size() - 1];
    if ((int)downflowmap.size() <= max_idx)
      downflowmap.resize(max_idx + 1);
    for (size_t i = 0; i < out_slots.size(); i++) {
      downflowmap[out_slots[i]].Init(
          map.thread_model,
          map.hold_input.size() > i ? map.hold_input[i] : HoldInputMode::NONE);
      out_slot_num++;
    }
  }
#ifndef NDEBUG
  c->SetMarkName(mark);
  c->SetExpectProcessTime(exp_process_time);
#else
  UNUSED(mark);
  UNUSED(exp_process_time);
#endif
  c->Start();
  return true;
}

void Flow::FlowMap::AddFlow(std::shared_ptr<Flow> flow, int index) {
  AutoLockMutex _lg(list_mtx);
  auto i = std::find(flows.begin(), flows.end(), flow);
  if (i != flows.end()) {
    LOG("repeatedly add, update index\n");
    i->index_of_in = index;
    return;
  }
  // TODO: sort by sync type in downflow
  flows.emplace_back(flow, index);
}

void Flow::FlowMap::RemoveFlow(std::shared_ptr<Flow> flow) {
  AutoLockMutex _lg(list_mtx);
  flows.remove_if([&flow](FlowInputMap &fm) { return fm == flow; });
}

bool Flow::AddDownFlow(std::shared_ptr<Flow> down, int out_slot_index,
                       int in_slot_index_of_down) {
  if (out_slot_num <= 0 || (int)downflowmap.size() != out_slot_num) {
    LOG("Uncompleted or final flow\n");
    return false;
  }
  if (out_slot_index >= (int)downflowmap.size()) {
    LOG("output slot index exceed max\n");
    return false;
  }
  if (this == down.get()) {
    LOG("can not set self loop flow\n");
    return false;
  }
  downflowmap[out_slot_index].AddFlow(down, in_slot_index_of_down);
  if (source_start_cond_mtx) {
    source_start_cond_mtx->lock();
    down_flow_num++;
    source_start_cond_mtx->notify();
    source_start_cond_mtx->unlock();
  }
  return true;
}

void Flow::RemoveDownFlow(std::shared_ptr<Flow> down) {
  if (out_slot_num <= 0 || (int)downflowmap.size() != out_slot_num)
    return;
  // if (down->down_flow_num > 0)
  //   LOG("the removing flow has down flows, remove them first\n");
  for (auto &dm : downflowmap) {
    if (!dm.valid)
      continue;
    dm.RemoveFlow(down);
    if (source_start_cond_mtx) {
      source_start_cond_mtx->lock();
      down_flow_num--;
      source_start_cond_mtx->notify();
      source_start_cond_mtx->unlock();
    }
  }
}

void Flow::SendInput(std::shared_ptr<MediaBuffer> &input, int in_slot_index) {
#ifndef NDEBUG
  if (in_slot_index < 0 || in_slot_index >= input_slot_num) {
    errno = EINVAL;
    return;
  }
#endif
  if (enable) {
    auto &in = v_input[in_slot_index];
    CALL_MEMBER_FN(in, in.send_input_behavior)(input);
  }
}

bool Flow::SetOutput(const std::shared_ptr<MediaBuffer> &output,
                     int out_slot_index) {
#ifndef NDEBUG
  if (out_slot_index < 0 || out_slot_index >= out_slot_num) {
    errno = EINVAL;
    return false;
  }
#endif
  if (enable) {
    auto &out = downflowmap[out_slot_index];
    CALL_MEMBER_FN(out, out.set_output_behavior)(output);
    return true;
  }
  return false;
}

bool Flow::ParseWrapFlowParams(const char *param,
                               std::map<std::string, std::string> &flow_params,
                               std::list<std::string> &sub_param_list) {
  sub_param_list = ParseFlowParamToList(param);
  if (sub_param_list.empty())
    return false;
  if (!parse_media_param_map(sub_param_list.front().c_str(), flow_params))
    return false;
  sub_param_list.pop_front();
  if (flow_params[KEY_NAME].empty()) {
    LOG("missing key name\n");
    return false;
  }
  return true;
}

void Flow::Input::SyncSendInputBehavior(std::shared_ptr<MediaBuffer> &input) {
  cached_buffer = input;
  coroutine->RunOnce();
}

void Flow::Input::ASyncSendInputCommonBehavior(
    std::shared_ptr<MediaBuffer> &input) {
  AutoLockMutex _alm(cond_mtx);
  if (max_cache_num > 0 && max_cache_num <= (int)cached_buffers.size()) {
    bool ret = (this->*async_full_behavior)(flow->enable);
    if (!ret)
      return;
  }
  cached_buffers.push_back(input);
  cond_mtx.notify();
}

void Flow::Input::ASyncSendInputAtomicBehavior(
    std::shared_ptr<MediaBuffer> &input) {
  AutoLockMutex _alm(spin_mtx);
  cached_buffer = input;
}

bool Flow::Input::ASyncFullBlockingBehavior(volatile bool &pred) {
  do {
    msleep(5);
    cond_mtx.unlock();
    if (max_cache_num > (int)cached_buffers.size()) {
      cond_mtx.lock();
      break;
    }
    cond_mtx.lock();
  } while (pred);

  return pred;
}

bool Flow::Input::ASyncFullDropFrontBehavior(volatile bool &pred _UNUSED) {
  cached_buffers.pop_front();
  return true;
}

bool Flow::Input::ASyncFullDropCurrentBehavior(volatile bool &pred _UNUSED) {
  return false;
}

std::string gen_datatype_rule(std::map<std::string, std::string> &params) {
  std::string rule;
  std::string value;
  CHECK_EMPTY_SETERRNO_RETURN(, value, params, KEY_INPUTDATATYPE, , "");
  PARAM_STRING_APPEND(rule, KEY_INPUTDATATYPE, value);
  CHECK_EMPTY_SETERRNO_RETURN(, value, params, KEY_OUTPUTDATATYPE, , "");
  PARAM_STRING_APPEND(rule, KEY_OUTPUTDATATYPE, value);
  return std::move(rule);
}

Model GetModelByString(const std::string &model) {
  static std::map<std::string, Model> model_map = {
      {KEY_ASYNCCOMMON, Model::ASYNCCOMMON},
      {KEY_ASYNCATOMIC, Model::ASYNCATOMIC},
      {KEY_SYNC, Model::SYNC}};
  auto it = model_map.find(model);
  if (it != model_map.end())
    return it->second;
  return Model::NONE;
}

InputMode GetInputModelByString(const std::string &in_model) {
  static std::map<std::string, InputMode> in_model_map = {
      {KEY_BLOCKING, InputMode::BLOCKING},
      {KEY_DROPFRONT, InputMode::DROPFRONT},
      {KEY_DROPCURRENT, InputMode::DROPCURRENT}};
  auto it = in_model_map.find(in_model);
  if (it != in_model_map.end())
    return it->second;
  return InputMode::NONE;
}

void ParseParamToSlotMap(std::map<std::string, std::string> &params,
                         SlotMap &sm, int &input_maxcachenum) {
  float fps = 0.0f;
  std::string &fps_str = params[KEY_FPS];
  if (!fps_str.empty()) {
    fps = std::stof(fps_str);
    if (fps > 0.0f)
      sm.interval = 1000.0f / fps;
  }
  sm.thread_model = GetModelByString(params[KEK_THREAD_SYNC_MODEL]);
  sm.mode_when_full = GetInputModelByString(params[KEK_INPUT_MODEL]);
  std::string &cache_num_str = params[KEY_INPUT_CACHE_NUM];
  int cache_num = -1;
  if (!cache_num_str.empty()) {
    cache_num = std::stoi(cache_num_str);
    if (cache_num <= 0)
      LOG("warning, input cache num = %d\n", cache_num);
    input_maxcachenum = cache_num;
  }
}

size_t FlowOutputHoldInput(std::shared_ptr<MediaBuffer> &out_buffer,
                           const MediaBufferVector &input_vector) {
  assert(out_buffer);
  size_t i = 0;
  for (; i < input_vector.size(); i++)
    out_buffer->SetRelatedSPtr(input_vector[i], i);
  return i;
}

size_t FlowOutputInheritFromInput(std::shared_ptr<MediaBuffer> &out_buffer,
                                  const MediaBufferVector &input_vector) {
  assert(out_buffer);
  size_t ret = 0;
  auto &vec = out_buffer->GetRelatedSPtrs();
  for (size_t i = 0; i < input_vector.size(); i++) {
    auto &input_vec = input_vector[i]->GetRelatedSPtrs();
    ret += input_vec.size();
    vec.insert(vec.end(), input_vec.begin(), input_vec.end());
  }
  return ret;
}

std::string JoinFlowParam(const std::string &flow_param, size_t num_elem, ...) {
  std::string ret;
  ret.append(flow_param);
  va_list va;
  va_start(va, num_elem);
  while (num_elem--) {
    const std::string &elem_param = va_arg(va, const std::string);
    ret.append(1, FLOW_PARAM_SEPARATE_CHAR).append(elem_param);
  }
  va_end(va);
  return std::move(ret);
}

std::list<std::string> ParseFlowParamToList(const char *param) {
  std::list<std::string> separate_list;
  if (!parse_media_param_list(param, separate_list, FLOW_PARAM_SEPARATE_CHAR) ||
      separate_list.size() < 2)
    separate_list.clear();
  return std::move(separate_list);
}

} // namespace easymedia
