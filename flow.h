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

#ifndef EASYMEDIA_FLOW_H_
#define EASYMEDIA_FLOW_H_

#include "lock.h"
#include "reflector.h"

#include <stdarg.h>

#include <deque>
#include <thread>
#include <type_traits>
#include <vector>

#include "control.h"

namespace easymedia {

DECLARE_FACTORY(Flow)
// usage: REFLECTOR(Flow)::Create<T>(flowname, param)
// T must be the final class type exposed to user
DECLARE_REFLECTOR(Flow)

#define DEFINE_FLOW_FACTORY(REAL_PRODUCT, FINAL_EXPOSE_PRODUCT)                \
  DEFINE_MEDIA_CHILD_FACTORY(REAL_PRODUCT, REAL_PRODUCT::GetFlowName(),        \
                             FINAL_EXPOSE_PRODUCT, Flow)                       \
  DEFINE_MEDIA_CHILD_FACTORY_EXTRA(REAL_PRODUCT)                               \
  DEFINE_MEDIA_NEW_PRODUCT_BY(REAL_PRODUCT, FINAL_EXPOSE_PRODUCT,              \
                              GetError() < 0)

class MediaBuffer;
enum class Model { NONE, ASYNCCOMMON, ASYNCATOMIC, SYNC };
// PushMode
enum class InputMode { NONE, BLOCKING, DROPFRONT, DROPCURRENT };
enum class HoldInputMode { NONE, HOLD_INPUT, INHERIT_FORM_INPUT };
using MediaBufferVector = std::vector<std::shared_ptr<MediaBuffer>>;
// TODO: outputs ret, outslot index, outslot queue model
using FunctionProcess =
    std::add_pointer<bool(Flow *f, MediaBufferVector &input_vector)>::type;
template <int in_index, int out_index>
bool void_transaction(Flow *f, MediaBufferVector &input_vector);

class _API SlotMap {
public:
  SlotMap()
      : thread_model(Model::SYNC), mode_when_full(InputMode::DROPFRONT),
        process(nullptr), interval(16.66f) {}
  std::vector<int> input_slots;
  Model thread_model;
  InputMode mode_when_full;
  std::vector<bool> fetch_block; // if ASYNCCOMMON
  std::vector<int> input_maxcachenum;
  std::vector<int> output_slots;
  // std::vector<DataSetModel> output_ds_model;
  std::vector<HoldInputMode> hold_input;
  FunctionProcess process;
  float interval;
};

class FlowCoroutine;
class _API Flow {
public:
  // We may need a flow which can be sync and async.
  // This make a side effect that sync flow contains superfluous variables
  // designed for async implementation.
  Flow();
  virtual ~Flow();
  static const char *GetFlowName() { return nullptr; }

  // TODO: Right now out_slot_index and in_slot_index is decided by exact
  //       subclass, automatically get these value or ignore them in future.
  bool AddDownFlow(std::shared_ptr<Flow> down, int out_slot_index,
                   int in_slot_index_of_down);
  void RemoveDownFlow(std::shared_ptr<Flow> down);

  void SendInput(std::shared_ptr<MediaBuffer> &input, int in_slot_index);
  void SetDisable() { enable = false; }

  // The Control must be called in the same thread to that create flow
  virtual int Control(unsigned long int request _UNUSED, ...) { return -1; }
  virtual int SubControl(unsigned long int request, void *arg) {
    SubRequest subreq = {request, arg};
    return Control(S_SUB_REQUEST, &subreq);
  }

  // The global event hander is the same thread to the born thread of this
  // object.
  // void SetEventHandler(EventHandler *ev_handler);
  // void NotifyToEventHandler();

protected:
  class FlowInputMap {
  public:
    FlowInputMap(std::shared_ptr<Flow> &f, int i) : flow(f), index_of_in(i) {}
    std::shared_ptr<Flow> flow; // weak_ptr?
    int index_of_in;
    bool operator==(const std::shared_ptr<easymedia::Flow> f) {
      return flow == f;
    }
  };
  class FlowMap {
  private:
    void SetOutputBehavior(const std::shared_ptr<MediaBuffer> &output);
    void SetOutputToQueueBehavior(const std::shared_ptr<MediaBuffer> &output);

  public:
    FlowMap() : valid(false), hold_input(HoldInputMode::NONE) {
      assert(list_mtx.valid);
    }
    FlowMap(FlowMap &&);
    void Init(Model m, HoldInputMode hold_in);
    bool valid;
    HoldInputMode hold_input;
    // down flow
    void AddFlow(std::shared_ptr<Flow> flow, int index);
    void RemoveFlow(std::shared_ptr<Flow> flow);
    std::list<FlowInputMap> flows;
    ReadWriteLockMutex list_mtx;
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
    Input() : valid(false), flow(nullptr), fetch_block(true) {}
    Input(Input &&);
    void Init(Flow *f, Model m, int mcn, InputMode im, bool f_block,
              std::shared_ptr<FlowCoroutine> fc);
    bool valid;
    Flow *flow;
    Model thread_model;
    bool fetch_block;
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

  // source flow
  bool SetAsSource(const std::vector<int> &input_slots,
                   const std::vector<int> &output_slots, FunctionProcess f,
                   const std::string &mark);
  bool InstallSlotMap(SlotMap &map, const std::string &mark,
                      int exp_process_time);
  bool SetOutput(const std::shared_ptr<MediaBuffer> &output,
                 int out_slot_index);
  bool ParseWrapFlowParams(const char *param,
                           std::map<std::string, std::string> &flow_params,
                           std::list<std::string> &sub_param_list);
  // As sub threads may call the variable of child class,
  // we should define this for child class when it deconstruct.
  void StopAllThread();
  bool IsEnable() { return enable; }

  template <int in_index, int out_index>
  friend bool void_transaction(Flow *f, MediaBufferVector &input_vector) {
    return f->SetOutput(input_vector[in_index], out_index);
  }
  static const FunctionProcess void_transaction00;

private:
  volatile bool enable;
  volatile bool quit;

  friend class FlowCoroutine;

  DEFINE_ERR_GETSET()
  DECLARE_PART_FINAL_EXPOSE_PRODUCT(Flow)
};

std::string gen_datatype_rule(std::map<std::string, std::string> &params);
Model GetModelByString(const std::string &model);
InputMode GetInputModelByString(const std::string &in_model);
void ParseParamToSlotMap(std::map<std::string, std::string> &params,
                         SlotMap &sm, int &input_maxcachenum);
size_t FlowOutputHoldInput(std::shared_ptr<MediaBuffer> &out_buffer,
                           const MediaBufferVector &input_vector);
size_t FlowOutputInheritFromInput(std::shared_ptr<MediaBuffer> &out_buffer,
                                  const MediaBufferVector &input_vector);

// the separator of flow params and flow core element params
#define FLOW_PARAM_SEPARATE_CHAR ' '
_API std::string JoinFlowParam(const std::string &flow_param, size_t num_elem,
                               ...);
_API std::list<std::string> ParseFlowParamToList(const char *param);

} // namespace easymedia

#endif // #ifndef EASYMEDIA_FLOW_H_
