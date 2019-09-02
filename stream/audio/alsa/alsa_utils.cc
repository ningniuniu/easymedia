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

#include "alsa_utils.h"

#include "key_string.h"
#include "utils.h"

static const struct SampleFormatEntry {
  SampleFormat fmt;
  snd_pcm_format_t alsa_fmt;
} sample_format_alsa_map[] = {
    {SAMPLE_FMT_U8, SND_PCM_FORMAT_U8},
    {SAMPLE_FMT_S16, SND_PCM_FORMAT_S16_LE},
    {SAMPLE_FMT_S32, SND_PCM_FORMAT_S32_LE},
};

snd_pcm_format_t SampleFormatToAlsaFormat(SampleFormat fmt) {
  FIND_ENTRY_TARGET(fmt, sample_format_alsa_map, fmt, alsa_fmt)
  return SND_PCM_FORMAT_UNKNOWN;
}

void ShowAlsaAvailableFormats(snd_pcm_t *handle, snd_pcm_hw_params_t *params) {
  snd_pcm_format_t format;
  printf("Available formats:\n");
  for (int i = 0; i <= SND_PCM_FORMAT_LAST; i++) {
    format = static_cast<snd_pcm_format_t>(i);
    if (snd_pcm_hw_params_test_format(handle, params, format) == 0)
      printf("- %s\n", snd_pcm_format_name(format));
  }
}

int ParseAlsaParams(const char *param,
                    std::map<std::string, std::string> &params,
                    std::string &device, SampleInfo &sample_info) {
  int ret = 0;
  if (!easymedia::parse_media_param_map(param, params))
    return 0;
  for (auto &p : params) {
    const std::string &key = p.first;
    if (key == KEY_SAMPLE_FMT) {
      SampleFormat fmt = StringToSampleFmt(p.second.c_str());
      if (fmt == SAMPLE_FMT_NONE) {
        LOG("unknown pcm fmt: %s\n", p.second.c_str());
        return 0;
      }
      sample_info.fmt = fmt;
      ret++;
    } else if (key == KEY_CHANNELS) {
      sample_info.channels = stoi(p.second);
      ret++;
    } else if (key == KEY_SAMPLE_RATE) {
      sample_info.sample_rate = stoi(p.second);
      ret++;
    } else if (key == KEY_DEVICE) {
      device = p.second;
    }
  }
  return ret;
}

// open device, and set format/channel/samplerate.
snd_pcm_t *AlsaCommonOpenSetHwParams(const char *device,
                                     snd_pcm_stream_t stream, int mode,
                                     SampleInfo &sample_info,
                                     snd_pcm_hw_params_t *hwparams) {
  snd_pcm_t *pcm_handle = NULL;
  unsigned int rate = sample_info.sample_rate;
  unsigned int channels;
  snd_pcm_format_t pcm_fmt = SampleFormatToAlsaFormat(sample_info.fmt);
  if (pcm_fmt == SND_PCM_FORMAT_UNKNOWN)
    return NULL;

  int status = snd_pcm_open(&pcm_handle, device, stream, mode);
  if (status < 0 || !pcm_handle) {
    LOG("audio open error: %s\n", snd_strerror(status));
    goto err;
  }

  status = snd_pcm_hw_params_any(pcm_handle, hwparams);
  if (status < 0) {
    LOG("Couldn't get hardware config: %s\n", snd_strerror(status));
    goto err;
  }
#ifndef NDEBUG
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

  return pcm_handle;

err:
  if (pcm_handle) {
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
  }
  return NULL;
}
