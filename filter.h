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

#ifndef EASYMEDIA_FILTER_H_
#define EASYMEDIA_FILTER_H_

#include <memory>

#include "media_reflector.h"

namespace easymedia {

DECLARE_FACTORY(Filter)

// usage: REFLECTOR(Filter)::Create<T>(filtername, param)
// T must be the final class type exposed to user
DECLARE_REFLECTOR(Filter)

#define DEFINE_FILTER_FACTORY(REAL_PRODUCT, FINAL_EXPOSE_PRODUCT)              \
  DEFINE_MEDIA_CHILD_FACTORY(REAL_PRODUCT, REAL_PRODUCT::GetFilterName(),      \
                             FINAL_EXPOSE_PRODUCT, Filter)                     \
  DEFINE_MEDIA_CHILD_FACTORY_EXTRA(REAL_PRODUCT)                               \
  DEFINE_MEDIA_NEW_PRODUCT_BY(REAL_PRODUCT, Filter, GetError() < 0)

#define DEFINE_COMMON_FILTER_FACTORY(REAL_PRODUCT)                             \
  DEFINE_FILTER_FACTORY(REAL_PRODUCT, Filter)

class MediaBuffer;
class _API Filter {
public:
  virtual ~Filter() = 0;
  static const char *GetFilterName() { return nullptr; }
  // sync call, input and output must be valid
  virtual int Process(std::shared_ptr<MediaBuffer> input,
                      std::shared_ptr<MediaBuffer> output);
  // some filter may output many buffers with one input.
  // sync or async safe call, depends on specific filter.
  virtual int SendInput(std::shared_ptr<MediaBuffer> input);
  virtual std::shared_ptr<MediaBuffer> FetchOutput();

  DEFINE_ERR_GETSET()
  DECLARE_PART_FINAL_EXPOSE_PRODUCT(Filter)
};

} // namespace easymedia

#endif // #ifndef EASYMEDIA_FILTER_H_
