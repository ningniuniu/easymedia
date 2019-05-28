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

#ifndef RKMEDIA_OGG_UTILS_H_
#define RKMEDIA_OGG_UTILS_H_

extern "C" {
#include <ogg/ogg.h>
}

#include <list>

namespace rkmedia {

bool PackOggPackets(const std::list<ogg_packet> &ogg_packets, void **out_buffer,
                    size_t *out_size);
bool UnpackOggData(void *in_buffer, size_t in_size,
                   std::list<ogg_packet> &ogg_packets);

ogg_packet *ogg_packet_clone(const ogg_packet &orig);
int ogg_packet_free(ogg_packet *p);

} // namespace rkmedia

#endif // #ifndef RKMEDIA_OGG_UTILS_H_
