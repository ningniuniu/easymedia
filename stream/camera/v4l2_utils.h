/*
 * Copyright (C) 2019 Hertz Wang 1989wanghang@163.com
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

#ifndef EASYMEDIA_V4L2_UTILS_H_
#define EASYMEDIA_V4L2_UTILS_H_

#include <linux/videodev2.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "key_string.h"
#include "media_type.h"
#include "utils.h"

namespace easymedia {

__u32 GetV4L2Type(const char *v4l2type);
__u32 GetV4L2FmtByString(const char *type);
__u32 GetV4L2ColorSpaceByString(const char *type);
const std::string &GetStringOfV4L2Fmts();

typedef struct {
  int (*open_f)(const char *file, int oflag, ...);
  int (*close_f)(int fd);
  int (*dup_f)(int fd);
  int (*ioctl_f)(int fd, unsigned long int request, ...);
  ssize_t (*read_f)(int fd, void *buffer, size_t n);
  void *(*mmap_f)(void *start, size_t length, int prot, int flags, int fd,
                  int64_t offset);
  int (*munmap_f)(void *_start, size_t length);
} v4l2_io;

bool SetV4L2IoFunction(v4l2_io *vio, bool use_libv4l2 = false);
int V4L2IoCtl(v4l2_io *vio, int fd, unsigned long int request, void *arg);

} // namespace easymedia

#endif // #ifndef EASYMEDIA_V4L2_UTILS_H_