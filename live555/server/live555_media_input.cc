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

#include "live555_media_input.hh"

#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>

#include "buffer.h"
#include "utils.h"

namespace easymedia {
// A common "FramedSource" subclass, used for reading from a cached buffer list:

class ListSource : public FramedSource {
protected:
  ListSource(UsageEnvironment &env, Live555MediaInput &input)
      : FramedSource(env), fInput(input), fReadFd(-1) {}
  virtual ~ListSource() {
    if (fReadFd >= 0)
      envir().taskScheduler().turnOffBackgroundReadHandling(fReadFd);
  }

  virtual bool readFromList(bool flush = false) = 0;
  virtual void flush();

  Live555MediaInput &fInput;
  int fReadFd;

private: // redefined virtual functions:
  virtual void doGetNextFrame();
  virtual void doStopGettingFrames();

  static void incomingDataHandler(ListSource *source, int mask);
  void incomingDataHandler1();
};

class VideoFramedSource : public ListSource {
public:
  VideoFramedSource(UsageEnvironment &env, Live555MediaInput &input);
  virtual ~VideoFramedSource();

protected: // redefined virtual functions:
  virtual bool readFromList(bool flush = false);
  bool got_iframe;
};

class AudioFramedSource : public ListSource {
public:
  AudioFramedSource(UsageEnvironment &env, Live555MediaInput &input);
  virtual ~AudioFramedSource();

protected: // redefined virtual functions:
  virtual bool readFromList(bool flush = false);
};

Live555MediaInput::Live555MediaInput(UsageEnvironment &env)
    : Medium(env), connecting(false), video_source(nullptr),
      audio_source(nullptr) {}

Live555MediaInput::~Live555MediaInput() { connecting = false; }

void Live555MediaInput::Start(UsageEnvironment &env _UNUSED) {
  connecting = true;
  LOG_FILE_FUNC_LINE();
}

void Live555MediaInput::Stop(UsageEnvironment &env _UNUSED) {
  connecting = false;
  // TODO: tell main to stop all up flow resource
  LOG_FILE_FUNC_LINE();
}

Live555MediaInput *Live555MediaInput::createNew(UsageEnvironment &env) {
  Live555MediaInput *mi = new Live555MediaInput(env);
  if (mi && !mi->initialize(env)) {
    delete mi;
    return nullptr;
  }
  return mi;
}

FramedSource *Live555MediaInput::videoSource() {
  if (!video_source)
    video_source = new VideoFramedSource(envir(), *this);
  return video_source;
}

FramedSource *Live555MediaInput::audioSource() {
  if (!audio_source)
    audio_source = new AudioFramedSource(envir(), *this);
  return audio_source;
}

Boolean Live555MediaInput::initialize(UsageEnvironment &env) {
  do {
    if (!initAudio(env))
      break;
    if (!initVideo(env))
      break;

    return True;
  } while (0);

  // An error occurred
  return False;
}

#if 0
static void printErr(UsageEnvironment& env, char const* str = NULL) {
  if (str != NULL)
    env << str;
  env << ": " << strerror(env.getErrno()) << "\n";
}
#endif

#define MAX_CACHE_NUMBER 60
static void common_reduction(std::list<std::shared_ptr<MediaBuffer>> &mb_list) {
  if (mb_list.size() > MAX_CACHE_NUMBER) {
    for (int i = 0; i < MAX_CACHE_NUMBER / 2; i++)
      mb_list.pop_front();
  }
}

Boolean Live555MediaInput::initAudio(UsageEnvironment &env _UNUSED) {
  as = std::make_shared<Source>();
  if (!as)
    return False;
  if (!as->Init(common_reduction)) {
    as.reset();
    return False;
  }
  return True;
}

static void
h264_packet_reduction(std::list<std::shared_ptr<MediaBuffer>> &mb_list) {
  if (mb_list.size() < MAX_CACHE_NUMBER)
    return;
  // only remain one I frame
  auto i = mb_list.rbegin();
  for (; i != mb_list.rend(); ++i) {
    auto &b = *i;
    if (b->GetUserFlag() & MediaBuffer::kIntra)
      break;
  }
  if (i == mb_list.rend())
    return;
  auto iter = mb_list.begin();
  for (; iter != mb_list.end(); ++iter) {
    auto &b = *iter;
    if (!(b->GetUserFlag() & MediaBuffer::kExtraIntra))
      break;
  }
  LOG("h264 reduction before, num: %d \n", (int)mb_list.size());
  mb_list.erase(iter, (++i).base());
  LOG("h264 reduction after, num: %d \n", (int)mb_list.size());
}

Boolean Live555MediaInput::initVideo(UsageEnvironment &env _UNUSED) {
  vs = std::make_shared<Source>();
  if (!vs)
    return False;
  if (!vs->Init(h264_packet_reduction)) {
    vs.reset();
    return False;
  }
  return True;
}

void Live555MediaInput::PushNewVideo(std::shared_ptr<MediaBuffer> &buffer) {
  if (!buffer)
    return;
  if (connecting || (buffer->GetUserFlag() & MediaBuffer::kExtraIntra))
    vs->Push(buffer);
}

void Live555MediaInput::PushNewAudio(std::shared_ptr<MediaBuffer> &buffer) {
  if (!buffer)
    return;
  if (connecting)
    as->Push(buffer);
}

Live555MediaInput::Source::Source() : reduction(nullptr) {
  wakeFds[0] = wakeFds[1] = -1;
}

Live555MediaInput::Source::~Source() {
  if (wakeFds[0] >= 0) {
    ::close(wakeFds[0]);
    wakeFds[0] = -1;
  }
  if (wakeFds[1] >= 0) {
    ::close(wakeFds[1]);
    wakeFds[1] = -1;
  }
  LOG("remain %d buffers, will auto release\n", (int)cached_buffers.size());
}

bool Live555MediaInput::Source::Init(ListReductionPtr func) {
  // create pipe fds
  int ret = pipe2(wakeFds, O_CLOEXEC);
  if (ret) {
    LOG("pipe2 failed: %m\n");
    return false;
  }
  assert(wakeFds[0] >= 0 && wakeFds[1] >= 0);
  reduction = func;
  return true;
}

void Live555MediaInput::Source::Push(std::shared_ptr<MediaBuffer> &buffer) {
  AutoLockMutex _alm(mtx);
  if (reduction)
    reduction(cached_buffers);
  cached_buffers.push_back(buffer);
  // mtx.notify();
  int i = 0;
  write(wakeFds[1], &i, sizeof(i));
}

std::shared_ptr<MediaBuffer> Live555MediaInput::Source::Pop() {
  AutoLockMutex _alm(mtx);
  if (cached_buffers.empty())
    return nullptr;
  auto buffer = cached_buffers.front();
  cached_buffers.pop_front();
  return std::move(buffer);
}

void ListSource::doGetNextFrame() {
  assert(fReadFd >= 0);
  // Await the next incoming data on our FID:
  envir().taskScheduler().turnOnBackgroundReadHandling(
      fReadFd, (TaskScheduler::BackgroundHandlerProc *)&incomingDataHandler,
      this);
}

void ListSource::doStopGettingFrames() {
  LOG_FILE_FUNC_LINE();
  FramedSource::doStopGettingFrames();
}

void ListSource::incomingDataHandler(ListSource *source, int /*mask*/) {
  source->incomingDataHandler1();
}

void ListSource::incomingDataHandler1() {
  // Read the data from our file into the client's buffer:
  readFromList();

  assert(fReadFd >= 0);
  // Stop handling any more input, until we're ready again:
  envir().taskScheduler().turnOffBackgroundReadHandling(fReadFd);

  // Tell our client that we have new data:
  afterGetting(this);
}

bool ListSource::readFromList(bool flush _UNUSED) { return false; }

void ListSource::flush() {
  readFromList(true);
  fFrameSize = 0;
  fNumTruncatedBytes = 0;
}

VideoFramedSource::VideoFramedSource(UsageEnvironment &env,
                                     Live555MediaInput &input)
    : ListSource(env, input), got_iframe(false) {
  fReadFd = input.vs->GetReadFd();
}

VideoFramedSource::~VideoFramedSource() {
  LOG_FILE_FUNC_LINE();
  fInput.video_source = NULL;
}

bool VideoFramedSource::readFromList(bool flush _UNUSED) {
#ifdef DEBUG_SEND
  fprintf(stderr, "$$$$ %s, %d\n", __func__, __LINE__);
#endif
  std::shared_ptr<MediaBuffer> buffer;
  int i = 0;
  ssize_t read_size = (ssize_t)sizeof(i);
  ssize_t ret = read(fReadFd, &i, sizeof(i));
  if (ret != read_size) {
    LOG("%s:%d, read from pipe error, %m\n", __func__, __LINE__);
    envir() << __LINE__ << " read from pipe error: " << errno << "\n";
    goto err;
  }

  buffer = fInput.vs->Pop();
  if (!got_iframe) {
    got_iframe = buffer->GetUserFlag() & MediaBuffer::kIntra;
    if (!got_iframe && !(buffer->GetUserFlag() & MediaBuffer::kExtraIntra))
      goto err;
  }
  if (buffer) {
    fPresentationTime = buffer->GetTimeVal();
#ifdef DEBUG_SEND
    envir() << "video frame time: " << (int)fPresentationTime.tv_sec << "s, "
            << (int)fPresentationTime.tv_usec << "us. \n";
#endif
    fFrameSize = buffer->GetValidSize();
#ifdef DEBUG_SEND
    envir() << "video frame size: " << fFrameSize << "\n";
#endif
    assert(fFrameSize > 0);
    uint8_t *p = (uint8_t *)buffer->GetPtr();
    assert(p[0] == 0);
    assert(p[1] == 0);
    if (p[2] == 0) {
      assert(p[3] == 1);
      read_size = 4;
    } else {
      assert(p[2] == 1);
      read_size = 3;
    }
    fFrameSize -= read_size;
    if (fFrameSize > fMaxSize) {
      LOG("%s : %d, fFrameSize(%d) > fMaxSize(%d)\n", __func__, __LINE__,
          fFrameSize, fMaxSize);
      fNumTruncatedBytes = fFrameSize - fMaxSize;
      fFrameSize = fMaxSize;
    } else {
      fNumTruncatedBytes = 0;
    }
    memcpy(fTo, p + read_size, fFrameSize);
    return true;
  }

err:
  fFrameSize = 0;
  fNumTruncatedBytes = 0;
  return false;
}

AudioFramedSource::AudioFramedSource(UsageEnvironment &env,
                                     Live555MediaInput &input)
    : ListSource(env, input) {
  fReadFd = input.as->GetReadFd();
}

AudioFramedSource::~AudioFramedSource() { fInput.audio_source = NULL; }

bool AudioFramedSource::readFromList(bool flush _UNUSED) {
#if 0
  assert(fFileNo > 0);
#ifdef DEBUG_SEND
  fprintf(stderr, "$$$$ %s, %d\n", __func__, __LINE__);
#endif
  do {
    // Note the timestamp and size:
    ssize_t read_size = (ssize_t)sizeof(struct timeval);
    ssize_t ret = read(fFileNo, &fPresentationTime, read_size);
    if (ret != read_size) {
      envir() << __LINE__ << "read from pipe error: " << errno << "\n";
      break;
    }
#ifdef DEBUG_SEND
    envir() << "audio read pipe frame time: " << (int)fPresentationTime.tv_sec
            << "s, " << (int)fPresentationTime.tv_usec << "us. \n";
#endif
    assert(sizeof(fFrameSize) >= sizeof(unsigned int));
    read_size = sizeof(unsigned int);
    ret = read(fFileNo, &fFrameSize, read_size);
    if (ret != read_size) {
      envir() << __LINE__ << "read from pipe error: " << errno << "\n";
      break;
    }
    assert(fFrameSize > 0);
#ifdef DEBUG_SEND
    envir() << "audio read pipe size: " << fFrameSize << "\n";
#endif
    if (flush) {
      char tmp[512];
      while (fFrameSize > 0) {
        read_size = std::min(sizeof(tmp), fFrameSize);
        ret = read(fFileNo, tmp, read_size);
        if (ret != read_size && errno != EAGAIN) {
          envir() << __LINE__ << "read from pipe error: " << errno << "\n";
          break;
        }
        fFrameSize -= ret;
      }
      break;
    }
    if (fFrameSize > fMaxSize) {
      fNumTruncatedBytes = fFrameSize - fMaxSize;
      fFrameSize = fMaxSize;
    } else {
      fNumTruncatedBytes = 0;
    }
    ret = read(fFileNo, fTo, fFrameSize);
    if ((unsigned)ret != fFrameSize) {
      envir() << __LINE__ << "read from pipe error: " << errno << "\n";
      break;
    }
#ifdef DEBUG_SEND
    fprintf(stderr, "$$$$ %s, %d\n", __func__, __LINE__);
#endif
    return true;
  } while (0);
  fFrameSize = 0;
  fNumTruncatedBytes = 0;
#endif
  return false;
}

} // namespace easymedia
