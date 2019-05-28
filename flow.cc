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
#include "utils.h"

namespace easymedia {

template <int in_index, int out_index>
static bool void_transaction(Flow *f, MediaBufferVector &input_vector) {
  f->SetOutput(input_vector[in_index], out_index);
  return true;
}

FunctionProcess void_transaction00 = void_transaction<0, 0>;

class FlowCoroutine {
public:
  FlowCoroutine(Flow *f, Model sync_model, FunctionProcess func, int64_t inter);
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

  void SendNullBufferDown(std::list<Flow::FlowInputMap> &flows);
  void SendBufferDown(Flow::FlowMap &fm, std::list<Flow::FlowInputMap> &flows,
                      bool process_ret);
  void SendBufferDownFromDeque(Flow::FlowMap &fm,
                               std::list<Flow::FlowInputMap> &flows,
                               bool process_ret);

  Flow *flow;
  Model model;
  int64_t interval;
  std::vector<int> in_slots;
  std::vector<int> out_slots;
  std::thread *th;
  FunctionProcess th_run;

  MediaBufferVector in_vector;
  decltype(&FlowCoroutine::SyncFetchInput) fetch_input_func;
  decltype(&FlowCoroutine::SendBufferDown) send_down_func;
#ifdef DEBUG
public:
  void SetMarkName(std::string s) { name = s; }
  void SetExpectProcessTime(int time) { expect_process_time = time; }

private:
  std::string name;
  int expect_process_time; // ms
#endif
};

FlowCoroutine::FlowCoroutine(Flow *f, Model sync_model, FunctionProcess func,
                             int64_t inter)
    : flow(f), model(sync_model), interval(inter), th(nullptr), th_run(func)
#ifdef DEBUG
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
#ifdef DEBUG
  LOG("%s quit\n", name.c_str());
#endif
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
    LOG("invalid model %d\n", model);
    return false;
  }
  if (need_thread) {
    th = new std::thread(func, this);
    if (!th) {
      errno = ENOMEM;
      return false;
    }
  }
  in_vector.resize(in_slots.size());
  return true;
}

#ifdef DEBUG
static void check_consume_time(const char *name, int expect, int exactly) {
  if (exactly > expect) {
    LOG("%s, expect consume %d ms, however %d ms\n", name, expect, exactly);
  }
}
#endif

void FlowCoroutine::RunOnce() {
  bool ret;
  (this->*fetch_input_func)(in_vector);
#ifdef DEBUG
  {
    AutoDuration ad;
#endif
    ret = (*th_run)(flow, in_vector);
#ifdef DEBUG
    if (expect_process_time > 0)
      check_consume_time(name.c_str(), expect_process_time,
                         (int)(ad.Get() / 1000));
  }
#endif // DEBUG
  for (auto &buffer : in_vector)
    buffer.reset();
  for (int idx : out_slots) {
    auto &fm = flow->downflowmap[idx];
    std::list<Flow::FlowInputMap> flows;
    fm.list_mtx.lock();
    flows = fm.flows;
    fm.list_mtx.unlock();
    (this->*send_down_func)(fm, flows, ret);
  }
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
    if (v.empty())
      input.cond_mtx.wait();
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

void FlowCoroutine::SendNullBufferDown(std::list<Flow::FlowInputMap> &flows) {
  std::shared_ptr<MediaBuffer> nullbuffer;
  for (auto &f : flows)
    f.flow->SendInput(nullbuffer, f.index_of_in);
}

void FlowCoroutine::SendBufferDown(Flow::FlowMap &fm,
                                   std::list<Flow::FlowInputMap> &flows,
                                   bool process_ret) {
  if (!process_ret) {
    SendNullBufferDown(flows);
    return;
  }
  for (auto &f : flows)
    f.flow->SendInput(fm.cached_buffer, f.index_of_in);
}

void FlowCoroutine::SendBufferDownFromDeque(
    Flow::FlowMap &fm, std::list<Flow::FlowInputMap> &flows, bool process_ret) {
  if (!process_ret) {
    SendNullBufferDown(flows);
    return;
  }
  assert(!fm.cached_buffers.empty());
  auto buffer = fm.cached_buffers.front();
  fm.cached_buffers.pop_front();
  for (auto &f : flows)
    f.flow->SendInput(buffer, f.index_of_in);
}

DEFINE_REFLECTOR(Flow)
DEFINE_FACTORY_COMMON_PARSE(Flow)
DEFINE_PART_FINAL_EXPOSE_PRODUCT(Flow, Flow)

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
  assert(!fm.valid && "Flow::FlowMap is not copyable and moveable");
}

void Flow::FlowMap::Init(Model m) {
  assert(!valid);
  valid = true;
  if (m == Model::ASYNCCOMMON)
    set_output_behavior = &FlowMap::SetOutputToQueueBehavior;
  else
    set_output_behavior = &FlowMap::SetOutputBehavior;
}

void Flow::FlowMap::SetOutputBehavior(std::shared_ptr<MediaBuffer> &output) {
  cached_buffer = output;
}
void Flow::FlowMap::SetOutputToQueueBehavior(
    std::shared_ptr<MediaBuffer> &output) {
  cached_buffers.push_back(output);
}

Flow::Input::Input(Input &&in) {
  assert(!in.valid && "Flow::Input is not copyable and moveable");
}

void Flow::Input::Init(Flow *f, Model m, int mcn, InputMode im,
                       std::shared_ptr<FlowCoroutine> fc) {
  assert(!valid);
  valid = true;
  flow = f;
  thread_model = m;
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
  }
}

bool Flow::InstallSlotMap(SlotMap &map, const std::string &mark,
                          int exp_process_time) {
  // parameters validity check
  auto &in_slots = map.input_slots;
  if (!check_slots(in_slots, "input"))
    return false;
  bool ret = true;
  for (int i : in_slots) {
    if (i >= (int)v_input.size())
      continue;
    if (v_input[i].valid) {
      LOG("input slot %d has been set\n");
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
      LOG("output slot %d has been set\n");
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
  int max_idx = in_slots[in_slots.size() - 1];
  if ((int)v_input.size() <= max_idx)
    v_input.resize(max_idx + 1);
  for (size_t i = 0; i < in_slots.size(); i++) {
    v_input[in_slots[i]].Init(
        this, map.thread_model,
        (map.thread_model == Model::ASYNCCOMMON) ? map.input_maxcachenum[i] : 0,
        map.mode_when_full, c);
    input_slot_num++;
  }
  if (!out_slots.empty()) {
    max_idx = out_slots[out_slots.size() - 1];
    if ((int)downflowmap.size() <= max_idx)
      downflowmap.resize(max_idx + 1);
    for (int i : out_slots) {
      downflowmap[i].Init(map.thread_model);
      out_slot_num++;
    }
  }
#ifdef DEBUG
  c->SetMarkName(mark);
  c->SetExpectProcessTime(exp_process_time);
#endif
  c->Start();
  return true;
}

void Flow::FlowMap::AddFlow(std::shared_ptr<Flow> flow, int index) {
  std::lock_guard<std::mutex> _lg(list_mtx);
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
  std::lock_guard<std::mutex> _lg(list_mtx);
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
#ifdef DEBUG
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

void Flow::SetOutput(std::shared_ptr<MediaBuffer> &output, int out_slot_index) {
#ifdef DEBUG
  if (out_slot_index < 0 || out_slot_index >= out_slot_num) {
    errno = EINVAL;
    return;
  }
#endif
  if (enable) {
    auto &out = downflowmap[out_slot_index];
    CALL_MEMBER_FN(out, out.set_output_behavior)(output);
  }
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

} // namespace easymedia
