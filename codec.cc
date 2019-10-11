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

#include "codec.h"

#include <sys/prctl.h>

#include "buffer.h"
#include "utils.h"

namespace easymedia {

Codec::Codec() { memset(&config, 0, sizeof(config)); }

Codec::~Codec() {}

std::shared_ptr<MediaBuffer> Codec::GetExtraData(void **data, size_t *size) {
  if (data && size && extra_data) {
    *data = extra_data->GetPtr();
    *size = extra_data->GetValidSize();
  }
  return extra_data;
}

bool Codec::SetExtraData(void *data, size_t size, bool realloc) {
  if (!realloc) {
    if (!extra_data) {
      extra_data = std::make_shared<MediaBuffer>();
      if (!extra_data)
        return false;
    }
    extra_data->SetPtr(data);
    extra_data->SetValidSize(size);
    extra_data->SetSize(size);
    extra_data->SetUserData(nullptr);
    return true;
  }

  if (!data || size == 0)
    return false;

  auto extradata = MediaBuffer::Alloc(size);
  if (!extradata || extradata->GetSize() < size) {
    LOG_NO_MEMORY();
    return false;
  }
  memcpy(extradata->GetPtr(), data, size);
  extradata->SetValidSize(size);
  extra_data = extradata;
  return true;
}

bool Codec::Init() { return false; }

// Copy from ffmpeg.
static const uint8_t *find_startcode_internal(const uint8_t *p,
                                              const uint8_t *end) {
  const uint8_t *a = p + 4 - ((intptr_t)p & 3);

  for (end -= 3; p < a && p < end; p++) {
    if (p[0] == 0 && p[1] == 0 && p[2] == 1)
      return p;
  }

  for (end -= 3; p < end; p += 4) {
    uint32_t x = *(const uint32_t *)p;
    //      if ((x - 0x01000100) & (~x) & 0x80008000) // little endian
    //      if ((x - 0x00010001) & (~x) & 0x00800080) // big endian
    if ((x - 0x01010101) & (~x) & 0x80808080) { // generic
      if (p[1] == 0) {
        if (p[0] == 0 && p[2] == 1)
          return p;
        if (p[2] == 0 && p[3] == 1)
          return p + 1;
      }
      if (p[3] == 0) {
        if (p[2] == 0 && p[4] == 1)
          return p + 2;
        if (p[4] == 0 && p[5] == 1)
          return p + 3;
      }
    }
  }

  for (end += 3; p < end; p++) {
    if (p[0] == 0 && p[1] == 0 && p[2] == 1)
      return p;
  }

  return end + 3;
}

const uint8_t *find_h264_startcode(const uint8_t *p, const uint8_t *end) {
  const uint8_t *out = find_startcode_internal(p, end);
  if (p < out && out < end && !out[-1])
    out--;
  return out;
}

std::list<std::shared_ptr<MediaBuffer>>
split_h264_separate(const uint8_t *buffer, size_t length, int64_t timestamp) {
  std::list<std::shared_ptr<MediaBuffer>> l;
  const uint8_t *p = buffer;
  const uint8_t *end = p + length;
  const uint8_t *nal_start = nullptr, *nal_end = nullptr;
  nal_start = find_h264_startcode(p, end);
  // 00 00 01 or 00 00 00 01
  size_t start_len = (nal_start[2] == 1 ? 3 : 4);
  for (;;) {
    if (nal_start == end)
      break;
    nal_start += start_len;
    nal_end = find_h264_startcode(nal_start, end);
    size_t size = nal_end - nal_start + start_len;
    uint8_t nal_type = (*nal_start) & 0x1F;
    uint32_t flag;
    switch (nal_type) {
    case 7:
    case 8:
      flag = MediaBuffer::kExtraIntra;
      break;
    case 5:
      flag = MediaBuffer::kIntra;
      break;
    case 1:
      flag = MediaBuffer::kPredicted;
      break;
    default:
      flag = 0;
    }
    auto sub_buffer = MediaBuffer::Alloc(size);
    if (!sub_buffer) {
      LOG_NO_MEMORY(); // fatal error
      l.clear();
      return l;
    }
    memcpy(sub_buffer->GetPtr(), nal_start - start_len, size);
    sub_buffer->SetValidSize(size);
    sub_buffer->SetUserFlag(flag);
    sub_buffer->SetUSTimeStamp(timestamp);
    l.push_back(sub_buffer);

    nal_start = nal_end;
  }
  return std::move(l);
}

} // namespace easymedia
