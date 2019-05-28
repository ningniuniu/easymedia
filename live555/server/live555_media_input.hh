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

#ifndef RKMEDIA_LIVE555_MEDIA_INPUT_HH_
#define RKMEDIA_LIVE555_MEDIA_INPUT_HH_

#include <list>
#include <memory>
#include <type_traits>

#include <liveMedia/MediaSink.hh>

#include "lock.h"

namespace rkmedia {

class MediaBuffer;
class VideoFramedSource;
class AudioFramedSource;
using ListReductionPtr = std::add_pointer<void(
    std::list<std::shared_ptr<MediaBuffer>> &mb_list)>::type;
class Live555MediaInput : public Medium {
public:
  static Live555MediaInput *createNew(UsageEnvironment &env);
  virtual ~Live555MediaInput();
  FramedSource *videoSource();
  FramedSource *audioSource();
  void Start(UsageEnvironment &env);
  void Stop(UsageEnvironment &env);

  void PushNewVideo(std::shared_ptr<MediaBuffer> &buffer);
  void PushNewAudio(std::shared_ptr<MediaBuffer> &buffer);

private:
  Live555MediaInput(UsageEnvironment &env);
  Boolean initialize(UsageEnvironment &env);
  Boolean initAudio(UsageEnvironment &env);
  Boolean initVideo(UsageEnvironment &env);

  class Source {
  public:
    Source();
    ~Source();
    bool Init(ListReductionPtr func = nullptr);
    void Push(std::shared_ptr<rkmedia::MediaBuffer> &);
    std::shared_ptr<MediaBuffer> Pop();
    int GetReadFd() { return wakeFds[0]; }
    int GetWriteFd() { return wakeFds[1]; }

  private:
    std::list<std::shared_ptr<MediaBuffer>> cached_buffers;
    ConditionLockMutex mtx;
    ListReductionPtr reduction;
    int wakeFds[2]; // Live555's EventTrigger is poor for multithread, use fds
  };
  std::shared_ptr<Source> vs, as;

  volatile bool connecting;
  VideoFramedSource *video_source;
  AudioFramedSource *audio_source;

  friend class VideoFramedSource;
  friend class AudioFramedSource;
};

// Functions to set the optimal buffer size for RTP sink objects.
// These should be called before each RTPSink is created.
#define AUDIO_MAX_FRAME_SIZE 204800
#define VIDEO_MAX_FRAME_SIZE (1920 * 1080 * 2)
inline void setAudioRTPSinkBufferSize() {
  OutPacketBuffer::maxSize = AUDIO_MAX_FRAME_SIZE;
}
inline void setVideoRTPSinkBufferSize() {
  OutPacketBuffer::maxSize = VIDEO_MAX_FRAME_SIZE;
}

} // namespace rkmedia

#endif // #ifndef RKMEDIA_LIVE555_MEDIA_INPUT_HH_
