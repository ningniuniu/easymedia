/*
 * Copyright (C) 2018 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_UTILS_H_
#define RKMEDIA_UTILS_H_

#define _UNUSED __attribute__((unused))

void LOG(const char *format, ...);

#define LOG_NO_MEMORY()                                                        \
  fprintf(stderr, "No memory %s: %d\n", __FUNCTION__, __LINE__)

#define UPALIGNTO(value, align) ((value + align - 1) & (~(align - 1)))

#define UPALIGNTO16(value) UPALIGNTO(value, 16)

template <typename T, typename TBase> class IsDerived {
public:
  static int t(TBase *base) { return 1; }
  static char t(void *t2) { return 0; }
  static const bool Result = (sizeof(int) == sizeof(t((T *)nullptr)));
};

#endif // #ifndef RKMEDIA_UTILS_H_
