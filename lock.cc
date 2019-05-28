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

#include "lock.h"

#include <assert.h>

namespace easymedia {

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

} // namespace easymedia
