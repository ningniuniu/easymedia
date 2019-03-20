/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_LOCK_H_
#define RKMEDIA_LOCK_H_

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace rkmedia {

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

} // namespace rkmedia

#endif // #ifndef RKMEDIA_LOCK_H_
