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

#ifndef EASYMEDIA_RGA_H_
#define EASYMEDIA_RGA_H_

#include "image.h"
namespace easymedia {

class ImageBuffer;
int rga_blit(std::shared_ptr<ImageBuffer> src, std::shared_ptr<ImageBuffer> dst,
             ImageRect *src_rect = nullptr, ImageRect *dst_rect = nullptr,
             int rotate = 0);

} // namespace easymedia

#endif // #ifndef EASYMEDIA_RGA_H_
