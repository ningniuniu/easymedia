/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: hertz.wang hertz.wong@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "codec.h"

#include <sys/prctl.h>

#include "buffer.h"
#include "utils.h"

namespace rkmedia {

Codec::~Codec() {}
bool Codec::Init() { return false; }
int Codec::Process(std::shared_ptr<HwMediaBuffer> input _UNUSED,
                   std::shared_ptr<HwMediaBuffer> output _UNUSED,
                   std::shared_ptr<HwMediaBuffer> extra_output _UNUSED) {
  return -1;
}

static void delete_thread(std::thread *&th) {
  if (th) {
    th->join();
    delete th;
    th = NULL;
  }
}

ThreadCodec::~ThreadCodec() {
  SetQuit(true);
  output_mtx.lock();
  output_cond.notify_one();
  output_mtx.unlock();
  if (input_th)
    delete_thread(input_th);
  if (output_th)
    delete_thread(output_th);
  input_list.clear();
  output_list.clear();
  extra_output_list.clear();
}

bool ThreadCodec::Init() {
  const char *prefix = th_name_prefix.c_str();

  input_th = new std::thread(&ThreadCodec::InputRun, this);
  if (!input_th) {
    LOG("Fail to create %sinput thread\n", prefix);
    return false;
  }

  output_th = new std::thread(&ThreadCodec::OutputRun, this);
  if (!output_th) {
    LOG("Fail to create %soutput thread\n", prefix);
    SetQuit(true);
    input_th->join();
    delete input_th;
    input_th = NULL;
    return false;
  }

  return true;
}

int ThreadCodec::Process(std::shared_ptr<HwMediaBuffer> input _UNUSED,
                         std::shared_ptr<HwMediaBuffer> output _UNUSED,
                         std::shared_ptr<HwMediaBuffer> extra_output _UNUSED) {
  return -1;
}

bool ThreadCodec::SendInput(std::shared_ptr<HwMediaBuffer> input) {
  std::lock_guard<std::mutex> _lg(input_mtx);
  if (!quit) {
    input_list.push_back(input);
    input_cond.notify_one();
    return true;
  }
  return false;
}

std::shared_ptr<HwMediaBuffer>
ThreadCodec::GetElement(std::list<std::shared_ptr<HwMediaBuffer>> &list,
                        std::mutex &mtx, std::condition_variable &cond,
                        bool wait) {
  std::unique_lock<std::mutex> _lk(mtx);
  if (list.empty()) {
    if (quit || !wait)
      return nullptr;
    if (wait)
      cond.wait(_lk);
    if (quit)
      return nullptr;
  }
  auto ret = list.front();
  list.pop_front();
  return ret;
}

std::shared_ptr<HwMediaBuffer> ThreadCodec::GetOutPut(bool wait) {
  return GetElement(output_list, output_mtx, output_cond, wait);
}

std::shared_ptr<HwMediaBuffer> ThreadCodec::GetExtraPut() {
  return GetElement(extra_output_list, output_mtx, output_cond, false);
}

int ThreadCodec::ProcessInput(std::shared_ptr<HwMediaBuffer> input _UNUSED) {
  return 0;
}

std::shared_ptr<HwMediaBuffer> ThreadCodec::GenEmptyOutPutBuffer() {
  return std::make_shared<HwMediaBuffer>(-1, nullptr, 0);
}

int ProcessOutput(std::shared_ptr<HwMediaBuffer> output _UNUSED,
                  std::shared_ptr<HwMediaBuffer> extra_output _UNUSED) {
  return 0;
}

void ThreadCodec::InputRun() {
  char thread_name[16];
  snprintf(thread_name, sizeof(thread_name), "%s%s", th_name_prefix.c_str(),
           __FUNCTION__);
  prctl(PR_SET_NAME, thread_name, 0, 0, 0);

  std::shared_ptr<HwMediaBuffer> pending_input;
  while (!quit) {
    std::shared_ptr<HwMediaBuffer> input;
    if (pending_input)
      input = pending_input;
    else
      input = GetElement(input_list, input_mtx, input_cond, true);
    if (!input)
      continue;
    int ret = ProcessInput(input);
    if (ret) {
      if (ret == -EAGAIN) {
        pending_input = input;
        continue;
      } else {
        LOG("ProcessInput ret : %d\n", ret);
        break;
      }
    }
    if (pending_input)
      pending_input.reset();
  }
  pending_input.reset();
  LOG("exit %s\n", thread_name);
}

void ThreadCodec::OutputRun() {
  char thread_name[16];
  snprintf(thread_name, sizeof(thread_name), "%s%s", th_name_prefix.c_str(),
           __FUNCTION__);
  prctl(PR_SET_NAME, thread_name, 0, 0, 0);

  std::shared_ptr<HwMediaBuffer> cache_output;
  std::shared_ptr<HwMediaBuffer> cache_extra_output;
  while (!quit) {
    std::shared_ptr<HwMediaBuffer> output;
    std::shared_ptr<HwMediaBuffer> extra_output;
    if (cache_output)
      output = cache_output;
    else
      output = GenEmptyOutPutBuffer();
    if (cache_extra_output)
      extra_output = cache_extra_output;
    else
      extra_output =
          ThreadCodec::GenEmptyOutPutBuffer(); // GenEmptyExtraOutPutBuffer?
    if (!output) {
      LOG_NO_MEMORY();
      break;
    }
    int ret = ProcessOutput(output, extra_output);
    if (ret && ret != -EAGAIN) {
      LOG("ProcessInput ret : %d\n", ret);
      break;
    }

    if (output->IsValid()) {
      std::unique_lock<std::mutex> _lk(output_mtx);
      output_list.push_back(output);
      if (extra_output->IsValid()) {
        extra_output_list.push_back(extra_output);
        if (cache_extra_output)
          cache_extra_output.reset();
      } else {
        cache_extra_output = extra_output;
      }
      output_cond.notify_one();
      if (cache_output)
        cache_output.reset();
    } else {
      cache_output = output;
    }
  }
  cache_output.reset();
  cache_extra_output.reset();
  LOG("exit %s\n", thread_name);
}

} // namespace rkmedia
