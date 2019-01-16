/*
 * Copyright (C) 2018 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "utils.h"

#include <stdarg.h>
#include <stdio.h>

void LOG(const char *format, ...) {
  char line[1024];
  va_list vl;
  va_start(vl, format);
  vsnprintf(line, sizeof(line), format, vl);
  va_end(vl);
}
