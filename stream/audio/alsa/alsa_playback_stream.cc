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

#include "stream.h"

#include <assert.h>
#include <errno.h>

#include "alsa_utils.h"
#include "media_type.h"
#include "utils.h"

namespace easymedia {

class AlsaPlayBackStream : public Stream {
public:
  static const int kPresetFrames = 1024;
  static const int kPresetSampleRate = 48000; // the same to asound.conf
  static const int kPresetMinBufferSize = 8192;
  AlsaPlayBackStream(const char *param);
  virtual ~AlsaPlayBackStream();
  static const char *GetStreamName() { return "alsa_playback_stream"; }
  virtual size_t Read(void *ptr _UNUSED, size_t size _UNUSED,
                      size_t nmemb _UNUSED) final {
    return 0;
  }
  virtual int Seek(int64_t offset _UNUSED, int whence _UNUSED) final {
    return -1;
  }
  virtual long Tell() final { return -1; }
  virtual size_t Write(const void *ptr, size_t size, size_t nmemb) final;
  virtual int Open() final;
  virtual int Close() final;

private:
  SampleInfo sample_info;
  std::string device;
  snd_pcm_t *alsa_handle;
  size_t frame_size;
};

AlsaPlayBackStream::AlsaPlayBackStream(const char *param)
    : alsa_handle(NULL), frame_size(0) {
  memset(&sample_info, 0, sizeof(sample_info));
  sample_info.fmt = SAMPLE_FMT_NONE;
  std::map<std::string, std::string> params;
  int ret = ParseAlsaParams(param, params, device, sample_info);
  UNUSED(ret);
  if (SampleInfoIsValid(sample_info))
    SetWriteable(true);
  else
    LOG("missing some necessary param\n");
}

AlsaPlayBackStream::~AlsaPlayBackStream() {
  if (alsa_handle)
    AlsaPlayBackStream::Close();
}

size_t AlsaPlayBackStream::Write(const void *ptr, size_t size, size_t nmemb) {
  size_t buffer_len = size * nmemb;
  snd_pcm_sframes_t frames =
      (size == frame_size ? nmemb : buffer_len / frame_size);
  while (frames > 0) {
    // SND_PCM_ACCESS_RW_INTERLEAVED
    int status = snd_pcm_writei(alsa_handle, ptr, frames);
    if (status < 0) {
      if (status == -EAGAIN) {
        /* Apparently snd_pcm_recover() doesn't handle this case - does it
         * assume snd_pcm_wait() above? */
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        errno = EAGAIN;
        return 0;
      }
      status = snd_pcm_recover(alsa_handle, status, 0);
      if (status < 0) {
        /* Hmm, not much we can do - abort */
        LOG("ALSA write failed (unrecoverable): %s\n", snd_strerror(status));
        errno = EIO;
        goto out;
      }
      errno = EIO;
      goto out;
    }
    frames -= status;
  }

out:
  return (buffer_len - frames * frame_size) / size;
}

static int ALSA_finalize_hardware(snd_pcm_t *pcm_handle, uint32_t samples,
                                  int sample_size,
                                  snd_pcm_hw_params_t *hwparams,
                                  snd_pcm_uframes_t *period_size) {
  int status;
  snd_pcm_uframes_t bufsize;
  int ret = 0;

  status = snd_pcm_hw_params(pcm_handle, hwparams);
  if (status < 0) {
    LOG("Couldn't set pcm_hw_params: %s\n", snd_strerror(status));
    ret = -1;
    goto err;
  }

  /* Get samples for the actual buffer size */
  status = snd_pcm_hw_params_get_buffer_size(hwparams, &bufsize);
  if (status < 0) {
    LOG("Couldn't get buffer size: %s\n", snd_strerror(status));
    ret = -1;
    goto err;
  }

#ifndef NDEBUG
  if (bufsize != samples * sample_size) {
    LOG("warning: bufsize != samples * %d; %lu != %u * %d\n", sample_size,
        bufsize, samples, sample_size);
  }
#else
  UNUSED(samples);
  UNUSED(sample_size);
#endif

err:
  snd_pcm_hw_params_get_period_size(hwparams, period_size, NULL);
#ifndef NDEBUG
  /* This is useful for debugging */
  do {
    unsigned int periods = 0;
    snd_pcm_hw_params_get_periods(hwparams, &periods, NULL);
    LOG("ALSA: period size = %ld, periods = %u, buffer size = %lu\n",
        *period_size, periods, bufsize);
  } while (0);
#endif
  return ret;
}

static int ALSA_set_period_size(snd_pcm_t *pcm_handle, uint32_t samples,
                                int sample_size, snd_pcm_hw_params_t *params,
                                snd_pcm_uframes_t *period_size) {
  int status;
  snd_pcm_hw_params_t *hwparams;
  snd_pcm_uframes_t frames;
  unsigned int periods;

  /* Copy the hardware parameters for this setup */
  snd_pcm_hw_params_alloca(&hwparams);
  snd_pcm_hw_params_copy(hwparams, params);

  frames = samples;
  status = snd_pcm_hw_params_set_period_size_near(pcm_handle, hwparams, &frames,
                                                  NULL);
  if (status < 0) {
    LOG("Couldn't set period size<%d> : %s\n", (int)frames,
        snd_strerror(status));
    return -1;
  }

  // when enable dmixer in asound.conf, rv1108 need large buffersize
  if (samples * sample_size < AlsaPlayBackStream::kPresetMinBufferSize)
    periods = (AlsaPlayBackStream::kPresetMinBufferSize + frames - 1) / frames;
  else
    periods = (samples * sample_size + frames - 1) / frames;

  status =
      snd_pcm_hw_params_set_periods_near(pcm_handle, hwparams, &periods, NULL);
  if (status < 0) {
    LOG("Couldn't set periods<%d> : %s\n", periods, snd_strerror(status));
    return -1;
  }

  return ALSA_finalize_hardware(pcm_handle, samples, sample_size, hwparams,
                                period_size);
}

static int ALSA_set_buffer_size(snd_pcm_t *pcm_handle, uint32_t samples,
                                int sample_size, snd_pcm_hw_params_t *params,
                                snd_pcm_uframes_t *period_size) {
  int status;
  snd_pcm_hw_params_t *hwparams;
  snd_pcm_uframes_t frames;
  /* Copy the hardware parameters for this setup */
  snd_pcm_hw_params_alloca(&hwparams);
  snd_pcm_hw_params_copy(hwparams, params);

  frames = samples * sample_size;
  if (frames < AlsaPlayBackStream::kPresetMinBufferSize)
    frames = AlsaPlayBackStream::kPresetMinBufferSize;
  status =
      snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hwparams, &frames);
  if (status < 0) {
    LOG("Couldn't set buffer size<%d> : %s\n", (int)frames,
        snd_strerror(status));
    return -1;
  }

  return ALSA_finalize_hardware(pcm_handle, samples, sample_size, hwparams,
                                period_size);
}

int AlsaPlayBackStream::Open() {
  if (device.empty())
    device = "default";
  snd_pcm_t *pcm_handle = NULL;
  snd_pcm_hw_params_t *hwparams = NULL;
  snd_pcm_sw_params_t *swparams = NULL;
  snd_pcm_uframes_t period_size = 0;
  uint32_t frames;
  if (!Writeable())
    return -1;
  int status = snd_pcm_hw_params_malloc(&hwparams);
  if (status < 0) {
    LOG("snd_pcm_hw_params_malloc failed\n");
    return -1;
  }
  status = snd_pcm_sw_params_malloc(&swparams);
  if (status < 0) {
    LOG("snd_pcm_sw_params_malloc failed\n");
    goto err;
  }
  pcm_handle = AlsaCommonOpenSetHwParams(
      device.c_str(), SND_PCM_STREAM_PLAYBACK, 0, sample_info, hwparams);
  if (!pcm_handle)
    goto err;
  frames = std::min<int>(kPresetFrames,
                         2 << (MATH_LOG2(sample_info.sample_rate *
                                         kPresetFrames / kPresetSampleRate) -
                               1));
  frame_size = GetSampleSize(sample_info);
  if (frame_size == 0)
    goto err;
  if (ALSA_set_period_size(pcm_handle, frames, frame_size, hwparams,
                           &period_size) < 0 &&
      ALSA_set_buffer_size(pcm_handle, frames, frame_size, hwparams,
                           &period_size) < 0) {
    goto err;
  }
  status = snd_pcm_sw_params_current(pcm_handle, swparams);
  if (status < 0) {
    LOG("Couldn't get alsa software config: %s\n", snd_strerror(status));
    goto err;
  }
  status = snd_pcm_sw_params_set_avail_min(pcm_handle, swparams, period_size);
  if (status < 0) {
    LOG("Couldn't set minimum available period_size <%d>: %s\n",
        (int)period_size, snd_strerror(status));
    goto err;
  }
  status = snd_pcm_sw_params_set_start_threshold(pcm_handle, swparams, 0);
  if (status < 0) {
    LOG("Unable to set start threshold mode for playback: %s\n",
        snd_strerror(status));
    goto err;
  }
  status = snd_pcm_sw_params(pcm_handle, swparams);
  if (status < 0) {
    LOG("Couldn't set software audio parameters: %s\n", snd_strerror(status));
    goto err;
  }
  /* Switch to blocking mode for playback */
  // snd_pcm_nonblock(pcm_handle, 0);

  snd_pcm_hw_params_free(hwparams);
  snd_pcm_sw_params_free(swparams);
  alsa_handle = pcm_handle;
  return 0;

err:
  if (hwparams)
    snd_pcm_hw_params_free(hwparams);
  if (swparams)
    snd_pcm_sw_params_free(swparams);
  if (pcm_handle) {
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
  }
  return -1;
}

int AlsaPlayBackStream::Close() {
  if (alsa_handle) {
    snd_pcm_drop(alsa_handle);
    snd_pcm_close(alsa_handle);
    alsa_handle = NULL;
    LOG("audio playback close done\n");
    return 0;
  }
  return -1;
}

DEFINE_STREAM_FACTORY(AlsaPlayBackStream, Stream)

const char *FACTORY(AlsaPlayBackStream)::ExpectedInputDataType() {
  return AUDIO_PCM;
}

const char *FACTORY(AlsaPlayBackStream)::OutPutDataType() { return nullptr; }

} // namespace easymedia
