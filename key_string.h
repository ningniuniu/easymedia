/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_MEDIA_KEY_STRING_H_
#define RKMEDIA_MEDIA_KEY_STRING_H_

#define KEY_PATH "path"
#define KEY_OPEN_MODE "mode"

#define KEY_CODEC_NAME "codecname"
#define KEY_CODEC_PARAM "codecparam"

#define KEY_INPUTDATATYPE "input_data_type"
#define KEY_OUTPUTDATATYPE "output_data_type"

// image info
#define KEY_PIXFMT "pixel_fomat"
#define KEY_BUFFER_WIDTH "width"
#define KEY_BUFFER_HEIGHT "height"
#define KEY_BUFFER_VIR_WIDTH "virtual_width"
#define KEY_BUFFER_VIR_HEIGHT "virtual_height"

// video info
#define KEY_COMPRESS_QP_INIT "qp_init"
#define KEY_COMPRESS_QP_STEP "qp_step"
#define KEY_COMPRESS_QP_MIN "qp_min"
#define KEY_COMPRESS_QP_MAX "qp_max"
#define KEY_COMPRESS_BITRATE "bitrate"
#define KEY_FPS "framerate"
#define KEY_LEVEL "level"
#define KEY_VIDEO_GOP "gop"
#define KEY_PROFILE "profile"
#define KEY_COMPRESS_RC_QUALITY "rc_quality"
#define KEY_COMPRESS_RC_MODE "rc_mode"

// audio info
#define KEY_CHANNELS "channel_num"
#define KEY_SAMPLE_RATE "sample_rate"
#define KEY_FRAMES "frame_num"
#define KEY_FLOAT_QUALITY "compress_quality"

// rtsp
#define KEY_PORT_NUM "portnum"
#define KEY_USERNAME "username"
#define KEY_USERPASSWORD "userpwd"
#define KEY_CHANNEL_NAME "channel_name"

#define KEY_MEM_TYPE "mem_type"
#define KEY_MEM_ION "ion"
#define KEY_MEM_DRM "drm"
#define KEY_MEM_SIZE_PERTIME "size_pertime"

#define KEY_LOOP_TIME "loop_time"

#endif // #ifndef RKMEDIA_MEDIA_KEY_STRING_H_
