/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "stream.h"

#include <assert.h>
#include <errno.h>

#include <thread>

#include "alsa_utils.h"
#include "media_type.h"
#include "utils.h"

namespace rkmedia {

class AlsaPlayBackStream : public Stream {
public:
  static const int kPresetSamples = 1024;
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

  // virtual bool Readable() final { return false; }
  // virtual bool Seekable() final { return false; }

private:
  SampleInfo sample_info;
  std::string device;
  snd_pcm_t *alsa_handle;
  size_t sample_size;
};

AlsaPlayBackStream::AlsaPlayBackStream(const char *param)
    : alsa_handle(NULL), sample_size(0) {
  sample_info.fmt = SAMPLE_FMT_NONE;
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params))
    return;
  int i = 3; // "format", "channels", "sample_rate"
  for (auto &p : params) {
    const std::string &key = p.first;
    if (key == "format") {
      SampleFormat fmt = StringToSampleFormat(p.second.c_str());
      if (fmt == SAMPLE_FMT_NONE) {
        LOG("unknown pcm fmt: %s\n", p.second.c_str());
        return;
      }
      sample_info.fmt = fmt;
      i--;
    } else if (key == "channels") {
      sample_info.channels = stoi(p.second);
      i--;
    } else if (key == "sample_rate") {
      sample_info.sample_rate = stoi(p.second);
      i--;
    } else if (key == "device") {
      device = p.second;
    }
  }
  if (i == 0)
    SetWriteable(true);
  else
    LOG("missing some necessary param\n");
}

AlsaPlayBackStream::~AlsaPlayBackStream() { assert(!alsa_handle); }

size_t AlsaPlayBackStream::Write(const void *ptr, size_t size, size_t nmemb) {
  size_t buffer_len = size * nmemb;
  snd_pcm_sframes_t frames =
      (size == sample_size ? nmemb : buffer_len / sample_size);
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
  return (buffer_len - frames * sample_size) / size;
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

#ifdef DEBUG
  if (bufsize != samples * sample_size) {
    LOG("warning: bufsize != samples * %d; %lu != %u * %d\n", sample_size,
        bufsize, samples, sample_size);
  }
#endif

err:
  snd_pcm_hw_params_get_period_size(hwparams, period_size, NULL);
#ifdef DEBUG
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
    LOG("Couldn't set period size<%d> : %s\n", frames, snd_strerror(status));
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
    LOG("Couldn't set buffer size<%d> : %s\n", frames, snd_strerror(status));
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
  unsigned int rate = sample_info.sample_rate;
  uint32_t samples;
  unsigned int channels;
  if (!Writeable())
    return -1;
  snd_pcm_format_t pcm_fmt = SampleFormatToAlsaFormat(sample_info.fmt);
  if (pcm_fmt == SND_PCM_FORMAT_UNKNOWN)
    return -1;
  int status =
      snd_pcm_open(&pcm_handle, device.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
  if (status < 0 || !pcm_handle) {
    LOG("audio open error: %s\n", snd_strerror(status));
    goto err;
  }
  snd_pcm_hw_params_alloca(&hwparams);
  snd_pcm_sw_params_alloca(&swparams);
  if (!hwparams || !swparams) {
    LOG("snd_pcm_(hw/sw)_params_alloca failed\n");
    goto err;
  }
  status = snd_pcm_hw_params_any(pcm_handle, hwparams);
  if (status < 0) {
    LOG("Couldn't get hardware config: %s\n", snd_strerror(status));
    goto err;
  }
#ifdef DEBUG
  {
    snd_output_t *log = NULL;
    snd_output_stdio_attach(&log, stderr, 0);
    // fprintf(stderr, "HW Params of device \"%s\":\n",
    //        snd_pcm_name(pcm_handle));
    LOG("--------------------\n");
    snd_pcm_hw_params_dump(hwparams, log);
    LOG("--------------------\n");
    snd_output_close(log);
  }
#endif
  // TODO: fixed SND_PCM_ACCESS_RW_INTERLEAVED
  status = snd_pcm_hw_params_set_access(pcm_handle, hwparams,
                                        SND_PCM_ACCESS_RW_INTERLEAVED);
  if (status < 0) {
    LOG("Couldn't set interleaved access: %s\n", snd_strerror(status));
    goto err;
  }
  status = snd_pcm_hw_params_set_format(pcm_handle, hwparams, pcm_fmt);
  if (status < 0) {
    LOG("Couldn't find any hardware audio formats\n");
    ShowAlsaAvailableFormats(pcm_handle, hwparams);
    goto err;
  }
  status = snd_pcm_hw_params_set_channels(pcm_handle, hwparams,
                                          sample_info.channels);
  if (status < 0) {
    LOG("Couldn't set audio channels<%d>: %s\n", sample_info.channels,
        snd_strerror(status));
    goto err;
  }
  status = snd_pcm_hw_params_get_channels(hwparams, &channels);
  if (status < 0 || channels != (unsigned int)sample_info.channels) {
    LOG("final channels do not match expected, %d != %d. resample require.\n",
        channels, sample_info.channels);
    goto err;
  }
  status = snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &rate, NULL);
  if (status < 0) {
    LOG("Couldn't set audio frequency<%d>: %s\n", sample_info.sample_rate,
        snd_strerror(status));
    goto err;
  }
  if (rate != (unsigned int)sample_info.sample_rate) {
    LOG("final sample rate do not match expected, %d != %d. resample "
        "require.\n",
        rate, sample_info.sample_rate);
    goto err;
  }
  samples = std::min<int>(
      kPresetSamples,
      2 << (MATH_LOG2(rate * kPresetSamples / kPresetSampleRate) - 1));
  sample_size = channels * (snd_pcm_format_physical_width(pcm_fmt) >> 3);
  if (ALSA_set_period_size(pcm_handle, samples, sample_size, hwparams,
                           &period_size) < 0 &&
      ALSA_set_buffer_size(pcm_handle, samples, sample_size, hwparams,
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
    LOG("Couldn't set minimum available period_size <%d>: %s\n", period_size,
        snd_strerror(status));
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

  alsa_handle = pcm_handle;
  return 0;

err:
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
std::shared_ptr<Stream>
    FACTORY(AlsaPlayBackStream)::NewProduct(const char *param) {
  std::shared_ptr<Stream> ret = std::make_shared<AlsaPlayBackStream>(param);
  if (ret && ret->Open() < 0)
    return nullptr;
  return ret;
}

const char *FACTORY(AlsaPlayBackStream)::ExpectedInputDataType() {
  return AUDIO_PCM;
}

const char *FACTORY(AlsaPlayBackStream)::OutPutDataType() { return nullptr; }

} // namespace rkmedia
