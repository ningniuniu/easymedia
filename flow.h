/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang hertz.wong@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_FLOW_H_
#define RKMEDIA_FLOW_H_

#include "lock.h"
#include "reflector.h"

#include <deque>
#include <thread>
#include <type_traits>
#include <vector>

namespace rkmedia {

DECLARE_FACTORY(Flow)
// usage: REFLECTOR(Flow)::Create<T>(flowname, param)
// T must be the final class type exposed to user
DECLARE_REFLECTOR(Flow)

#define DEFINE_FLOW_FACTORY(REAL_PRODUCT, FINAL_EXPOSE_PRODUCT)                \
  DEFINE_MEDIA_CHILD_FACTORY(REAL_PRODUCT, REAL_PRODUCT::GetFlowName(),        \
                             FINAL_EXPOSE_PRODUCT, Flow)                       \
  DEFINE_MEDIA_CHILD_FACTORY_EXTRA(REAL_PRODUCT)                               \
  DEFINE_MEDIA_NEW_PRODUCT(REAL_PRODUCT, FINAL_EXPOSE_PRODUCT)

class MediaBuffer;
enum class Model { NONE, ASYNCCOMMON, ASYNCATOMIC, SYNC };
enum class InputMode { BLOCKING, DROPFRONT, DROPCURRENT };
using MediaBufferVector = std::vector<std::shared_ptr<MediaBuffer>>;
using FunctionProcess =
    std::add_pointer<bool(Flow *f, MediaBufferVector &input_vector)>::type;

extern FunctionProcess void_transaction00;

class SlotMap {
public:
  SlotMap() : interval(16) {}
  std::vector<int> input_slots;
  std::vector<int> output_slots;
  FunctionProcess process;
  Model thread_model;
  InputMode mode_when_full;
  std::vector<int> input_maxcachenum;
  int64_t interval;
};

class FlowCoroutine;
class Flow {
public:
  // We may need a flow which can be sync and async.
  // This make a side effect that sync flow contains superfluous variables
  // designed for async implementation.
  Flow();
  virtual ~Flow();
  static const char *GetFlowName() { return nullptr; }
  // source flow
  bool SetAsSource() {
    source_start_cond_mtx = std::make_shared<ConditionLockMutex>();
    return !!source_start_cond_mtx;
  }
  bool InstallSlotMap(SlotMap &map, const std::string &mark,
                      int exp_process_time);

  // TODO: Right now out_slot_index and in_slot_index is decided by exact
  //       subclass, automatically get these value or ignore them in future.
  bool AddDownFlow(std::shared_ptr<Flow> down, int out_slot_index,
                   int in_slot_index_of_down);
  void RemoveDownFlow(std::shared_ptr<Flow> down);

  void SendInput(std::shared_ptr<MediaBuffer> &input, int in_slot_index);
  void SetDisable() { enable = false; }

  // The global event hander is the same thread to the born thread of this
  // object
  // void SetEventHandler(EventHandler *ev_handler);
  // void NotifyToEventHandler();

  // protected
  void SetOutput(std::shared_ptr<MediaBuffer> &output, int out_slot_index);

protected:
  class FlowInputMap {
  public:
    FlowInputMap(std::shared_ptr<Flow> &f, int i) : flow(f), index_of_in(i) {}
    std::shared_ptr<Flow> flow; // weak_ptr?
    int index_of_in;
    bool operator==(const std::shared_ptr<rkmedia::Flow> f) {
      return flow == f;
    }
  };
  class FlowMap {
  private:
    void SetOutputBehavior(std::shared_ptr<MediaBuffer> &output);
    void SetOutputToQueueBehavior(std::shared_ptr<MediaBuffer> &output);

  public:
    FlowMap() : valid(false) {}
    FlowMap(FlowMap &&);
    void Init(Model m);
    bool valid;
    // down flow
    void AddFlow(std::shared_ptr<Flow> flow, int index);
    void RemoveFlow(std::shared_ptr<Flow> flow);
    std::list<FlowInputMap> flows;
    std::mutex list_mtx;
    std::deque<std::shared_ptr<MediaBuffer>> cached_buffers; // never drop
    std::shared_ptr<MediaBuffer> cached_buffer;
    decltype(&FlowMap::SetOutputBehavior) set_output_behavior;
  };
  class Input {
  private:
    void SyncSendInputBehavior(std::shared_ptr<MediaBuffer> &input);
    void ASyncSendInputCommonBehavior(std::shared_ptr<MediaBuffer> &input);
    void ASyncSendInputAtomicBehavior(std::shared_ptr<MediaBuffer> &input);
    // behavior when input list exceed max_cache_num
    bool ASyncFullBlockingBehavior(volatile bool &pred);
    bool ASyncFullDropFrontBehavior(volatile bool &pred);
    bool ASyncFullDropCurrentBehavior(volatile bool &pred);

  public:
    Input() : valid(false), flow(nullptr) {}
    Input(Input &&);
    void Init(Flow *f, Model m, int mcn, InputMode im,
              std::shared_ptr<FlowCoroutine> fc);
    bool valid;
    Flow *flow;
    Model thread_model;
    std::deque<std::shared_ptr<MediaBuffer>> cached_buffers;
    ConditionLockMutex cond_mtx;
    int max_cache_num;
    InputMode mode_when_full;
    std::shared_ptr<MediaBuffer> cached_buffer;
    SpinLockMutex spin_mtx;
    decltype(&Input::SyncSendInputBehavior) send_input_behavior;
    decltype(&Input::ASyncFullBlockingBehavior) async_full_behavior;
    std::shared_ptr<FlowCoroutine> coroutine;
  };

  // Can not change the following values after initialize,
  // except AddFlow and RemoveFlow.
  int out_slot_num;
  std::vector<FlowMap> downflowmap;
  int input_slot_num;
  std::vector<Input> v_input;
  std::list<std::shared_ptr<FlowCoroutine>> coroutines;
  std::shared_ptr<ConditionLockMutex> source_start_cond_mtx;
  int down_flow_num;

  // As sub threads may call the variable of child class,
  // we should define this for child class when it deconstruct.
  void StopAllThread();

private:
  volatile bool enable;
  volatile bool quit;

  friend class FlowCoroutine;

  DECLARE_PART_FINAL_EXPOSE_PRODUCT(Flow)
};

} // namespace rkmedia

#endif // #ifndef RKMEDIA_FLOW_H_
