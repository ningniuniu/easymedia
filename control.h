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

#ifndef EASYMEDIA_CONTROL_H_
#define EASYMEDIA_CONTROL_H_

#include <stdint.h>

namespace easymedia {

typedef struct {
  const char *name;
  uint64_t value;
} DRMPropertyArg;

typedef struct {
  unsigned long int sub_request;
  void *arg;
} SubRequest;

enum {
  S_FIRST_CONTROL = 10000,
  S_SUB_REQUEST, // many devices have their kernel controls
  // ImageRect
  S_SOURCE_RECT,
  S_DESTINATION_RECT,
  S_SRC_DST_RECT,
  // ImageInfo
  G_PLANE_IMAGE_INFO,
  // int
  G_PLANE_SUPPORT_SCALE,
  // DRMPropertyArg
  S_CRTC_PROPERTY,
  S_CONNECTOR_PROPERTY,
  // any type
  S_STREAM_OFF,
};

} // namespace easymedia

#endif // #ifndef EASYMEDIA_CONTROL_H_