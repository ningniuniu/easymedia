/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
