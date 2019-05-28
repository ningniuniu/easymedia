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

#ifndef RKMEDIA_LOCK_H_
#define RKMEDIA_LOCK_H_

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace easymedia {

class LockMutex {
public:
  LockMutex();
  virtual ~LockMutex();
  virtual void lock() = 0;
  virtual void unlock() = 0;
  virtual void wait() = 0;
  virtual void notify() = 0;
  void locktimeinc();
  void locktimedec();
#ifdef DEBUG
protected:
  int lock_times;
#endif
};

class NonLockMutex : public LockMutex {
public:
  virtual ~NonLockMutex() = default;
  virtual void lock() {}
  virtual void unlock() {}
  virtual void wait() {}
  virtual void notify() {}
};

class ConditionLockMutex : public LockMutex {
public:
  virtual ~ConditionLockMutex() = default;
  virtual void lock() override;
  virtual void unlock() override;
  virtual void wait() override;
  virtual void notify() override;

private:
  std::mutex mtx;
  std::condition_variable_any cond;
};

class SpinLockMutex : public LockMutex {
public:
  SpinLockMutex();
  virtual ~SpinLockMutex() = default;
  SpinLockMutex(const SpinLockMutex &) = delete;
  SpinLockMutex &operator=(const SpinLockMutex &) = delete;
  virtual void lock() override;
  virtual void unlock() override;
  virtual void wait() override {}
  virtual void notify() override {}

private:
  std::atomic_flag flag;
};

class AutoLockMutex {
public:
  AutoLockMutex(LockMutex &lm) : m_lm(lm) { m_lm.lock(); }
  ~AutoLockMutex() { m_lm.unlock(); }

private:
  LockMutex &m_lm;
};

} // namespace easymedia

#endif // #ifndef RKMEDIA_LOCK_H_
