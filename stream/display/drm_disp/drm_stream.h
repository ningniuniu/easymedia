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

#ifndef EASYMEDIA_DRM_STREAM_H_
#define EASYMEDIA_DRM_STREAM_H_

#include <assert.h>

#include <vector>

#include "drm_utils.h"
#include "image.h"
#include "stream.h"

namespace easymedia {

class DRMStream : public Stream {
public:
  DRMStream(const char *param, bool accept_scale = false);
  virtual ~DRMStream() { DRMStream::Close(); }
  virtual size_t Read(void *ptr _UNUSED, size_t size _UNUSED,
                      size_t nmemb _UNUSED) final {
    return 0;
  }
  virtual int Seek(int64_t offset _UNUSED, int whence _UNUSED) final {
    return -1;
  }
  virtual long Tell() final { return -1; }
  virtual size_t Write(const void *ptr _UNUSED, size_t size _UNUSED,
                       size_t nmemb _UNUSED) final {
    return 0;
  }

protected:
  virtual int Open() override;
  virtual int Close() override;

  void SetModeInfo(const drmModeModeInfo &mode);
  bool GetAgreeableIDSet();

  bool accept_scale;
  std::string device;
  std::string data_type;
  ImageInfo img_info;
  int fps;
  uint32_t connector_id; // plane_id is more preferred than connector_id
  uint32_t crtc_id;
  uint32_t encoder_id; // encoder_id may be covered by other value
  std::string plane_type;
  uint32_t plane_id;
  std::vector<uint32_t> skip_plane_ids;
  bool active;
  drmModeModeInfo cur_mode;
  uint32_t drm_fmt;
  bool find_strict_match_wh; // if set hdisplay/vdisplay, show whether found a
                             // modeinfo match

  int fd; // just for convenience
  struct resources *res;
  std::shared_ptr<DRMDevice> dev;
};

} // namespace easymedia

#endif // #ifndef EASYMEDIA_DRM_STREAM_H_
