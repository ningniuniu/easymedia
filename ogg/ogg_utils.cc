/*
 * Copyright (C) 2017 Hertz Wang wangh@rock-chips.com
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
 * Commercial non-GPL licensing of this software is possible.
 * For more info contact Rockchip Electronics Co., Ltd.
 */

#include "ogg_utils.h"

#include <assert.h>
#include <string.h>

#include "utils.h"

namespace rkmedia {

bool PackOggPackets(const std::list<ogg_packet> &ogg_packets, void **out_buffer,
                    size_t *out_size) {
  size_t total_size = 0;
  for (const ogg_packet &packet : ogg_packets)
    total_size += (sizeof(ogg_packet) + packet.bytes);
  if (total_size == 0)
    return false;
  void *out = *out_buffer;
  if (!out) {
    out = malloc(total_size);
    if (!out) {
      LOG_NO_MEMORY();
      return false;
    }
    *out_buffer = out;
    *out_size = total_size;
  } else {
    if (*out_size < total_size) {
      LOG("buffer is too small to pack all ogg packets.\n");
      return false;
    }
  }
  unsigned char *buffer = (unsigned char *)out;
  for (ogg_packet packet : ogg_packets) {
    unsigned char *packet_data = packet.packet;
    packet.packet = buffer + sizeof(packet);
    memcpy(buffer, &packet, sizeof(packet));
    buffer += sizeof(packet);
    memcpy(buffer, packet_data, packet.bytes);
    buffer += packet.bytes;
  }
  return true;
}

bool UnpackOggData(void *in_buffer, size_t in_size,
                   std::list<ogg_packet> &ogg_packets) {
  if (!in_buffer || in_size < sizeof(ogg_packet))
    return false;
  unsigned char *buffer = (unsigned char *)in_buffer;
  size_t offset = 0;
  while (offset < in_size) {
    ogg_packet packet = *((ogg_packet *)(buffer + offset));
    assert(packet.packet == buffer + offset + sizeof(ogg_packet));
    offset += (sizeof(ogg_packet) + packet.bytes);
    ogg_packets.push_back(std::move(packet));
  }
  return true;
}

ogg_packet *ogg_packet_clone(const ogg_packet &orig) {
  if (orig.bytes <= 0)
    return nullptr;
  ogg_packet *new_packet = (ogg_packet *)malloc(sizeof(ogg_packet));
  if (!new_packet)
    return nullptr;
  *new_packet = orig;
  void *buffer = malloc(orig.bytes);
  if (!buffer) {
    free(new_packet);
    return nullptr;
  }
  memcpy(buffer, orig.packet, orig.bytes);
  new_packet->packet = (unsigned char *)buffer;

  return new_packet;
}

int ogg_packet_free(ogg_packet *p) {
  if (!p)
    return -1;
  if (p->packet)
    free(p->packet);
  free(p);
  return 0;
}

} // namespace rkmedia
