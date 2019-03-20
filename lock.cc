/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "lock.h"

#include <assert.h>

namespace rkmedia {

LockMutex::LockMutex()
#ifdef DEBUG
    : lock_times(0)
#endif
{
}
LockMutex::~LockMutex() {
#ifdef DEBUG
  assert(lock_times == 0 && "mutex lock/unlock mismatch");
#endif
}

void LockMutex::locktimeinc() {
#ifdef DEBUG
  lock_times++;
#endif
}
void LockMutex::locktimedec() {
#ifdef DEBUG
  lock_times--;
#endif
}

void ConditionLockMutex::lock() {
  mtx.lock();
  locktimeinc();
}
void ConditionLockMutex::unlock() {
  locktimedec();
  mtx.unlock();
}
void ConditionLockMutex::wait() { cond.wait(mtx); }
void ConditionLockMutex::notify() { cond.notify_all(); }

SpinLockMutex::SpinLockMutex() : flag(ATOMIC_FLAG_INIT) {}
void SpinLockMutex::lock() {
  while (flag.test_and_set(std::memory_order_acquire))
    ;
  locktimeinc();
}
void SpinLockMutex::unlock() {
  locktimedec();
  flag.clear(std::memory_order_release);
}

} // namespace rkmedia
