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

#include "decoder.h"

namespace easymedia {

DEFINE_REFLECTOR(Decoder)

// request should equal codec_name
DEFINE_FACTORY_COMMON_PARSE(Decoder)

DEFINE_PART_FINAL_EXPOSE_PRODUCT(AudioDecoder, Decoder)
DEFINE_PART_FINAL_EXPOSE_PRODUCT(VideoDecoder, Decoder)

} // namespace easymedia
