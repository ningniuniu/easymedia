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

#include "filter.h"

namespace easymedia {

DEFINE_REFLECTOR(Filter)

// request should equal filter_name
DEFINE_FACTORY_COMMON_PARSE(Filter)

DEFINE_PART_FINAL_EXPOSE_PRODUCT(Filter, Filter)

Filter::~Filter() {}

int Filter::Process(std::shared_ptr<MediaBuffer> input _UNUSED,
                    std::shared_ptr<MediaBuffer> output _UNUSED) {
  errno = ENOSYS;
  return -1;
}

int Filter::SendInput(std::shared_ptr<MediaBuffer> input _UNUSED) {
  errno = ENOSYS;
  return -1;
}

std::shared_ptr<MediaBuffer> Filter::FetchOutput() {
  errno = ENOSYS;
  return nullptr;
}

} // namespace easymedia
