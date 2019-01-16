/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_TEST_UTILS_H_
#define RKMEDIA_TEST_UTILS_H_

#include "buffer.h"

// alloc hardware memory
std::shared_ptr<rkmedia::HwMediaBuffer> alloc_hw_memory(ImageInfo &info,
                                                        int num, int den);

#endif // #ifdef RKMEDIA_TEST_UTILS_H_
