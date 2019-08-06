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

#ifndef EASYMEDIA_MEDIA_KEY_STRING_H_
#define EASYMEDIA_MEDIA_KEY_STRING_H_

#define _STR(s) #s
#define STR(s) _STR(s)

#define KEY_PATH "path"
#define KEY_OPEN_MODE "mode"
#define KEY_DEVICE "device"

#define KEY_NAME "name"
#define KEY_INPUTDATATYPE "input_data_type"
#define KEY_OUTPUTDATATYPE "output_data_type"

// image info
#define KEY_PIXFMT "pixel_fomat"
#define KEY_BUFFER_WIDTH "width"
#define KEY_BUFFER_HEIGHT "height"
#define KEY_BUFFER_VIR_WIDTH "virtual_width"
#define KEY_BUFFER_VIR_HEIGHT "virtual_height"

// (src_left, src_top, src_width, src_height)->(dst_left, dst_top, dst_width,
// dst_height)
#define KEY_RIGHT_DIRECTION "->"
#define KEY_BUFFER_RECT "rect"
#define KEY_BUFFER_ROTATE "rotate"

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
#define KEY_NEED_EXTRA_OUTPUT "need_extra_output"

#define KEY_WORST "worst"
#define KEY_WORSE "worse"
#define KEY_MEDIUM "medium"
#define KEY_BETTER "better"
#define KEY_BEST "best"
#define KEY_CQP "cqp"
#define KEY_AQ_ONLY "aq_only"

#define KEY_VBR "vbr"
#define KEY_CBR "cbr"

// mpp special
#define KEY_MPP_GROUP_MAX_FRAMES "fg_max_frames" // framegroup max frame num
#define KEY_MPP_SPLIT_MODE "split_mode"
#define KEY_OUTPUT_TIMEOUT "output_timeout"

// audio info
#define KEY_SAMPLE_FMT "sample_format"
#define KEY_CHANNELS "channel_num"
#define KEY_SAMPLE_RATE "sample_rate"
#define KEY_FRAMES "frame_num"
#define KEY_FLOAT_QUALITY "compress_quality"

// v4l2 info
#define KEY_USE_LIBV4L2 "use_libv4l2"
#define KEY_SUB_DEVICE "sub_device"
#define KEY_V4L2_CAP_TYPE "v4l2_capture_type"
#define KEY_V4L2_C_TYPE(t) STR(t)
#define KEY_V4L2_MEM_TYPE "v4l2_mem_type"
#define KEY_V4L2_M_TYPE(t) STR(t)
#define KEY_V4L2_COLORSPACE "v4l2_colorspace"
#define KEY_V4L2_CS(t) STR(t)

// rtsp
#define KEY_PORT_NUM "portnum"
#define KEY_USERNAME "username"
#define KEY_USERPASSWORD "userpwd"
#define KEY_CHANNEL_NAME "channel_name"

#define KEY_MEM_TYPE "mem_type"
#define KEY_MEM_ION "ion"
#define KEY_MEM_DRM "drm"
#define KEY_MEM_HARDWARE "hw_mem"

#define KEY_MEM_SIZE_PERTIME "size_pertime"

#define KEY_LOOP_TIME "loop_time"

// flow
#define KEK_THREAD_SYNC_MODEL "thread_model"
#define KEY_ASYNCCOMMON "asynccommon"
#define KEY_ASYNCATOMIC "asyncatomic"
#define KEY_SYNC "sync"

#define KEK_INPUT_MODEL "input_model"
#define KEY_BLOCKING "blocking"
#define KEY_DROPFRONT "dropfront"
#define KEY_DROPCURRENT "dropcurrent"

#define KEY_INPUT_CACHE_NUM "input_cache_num"
#define KEY_OUTPUT_CACHE_NUM "output_cache_num"

#define KEY_OUTPUT_HOLD_INPUT "output_hold_input"

// drm
#define KEY_CONNECTOR_ID "connector_id"
#define KEY_CRTC_ID "crtc_id"
#define KEY_ENCODER_ID "encoder_id"
#define KEY_PLANE_ID "plane_id"
#define KEY_SKIP_PLANE_IDS "skip_plane_ids"
#define KEY_PLANE_TYPE "plane_type"
#define KEY_OVERLAY "Overlay"
#define KEY_PRIMARY "Primary"
#define KEY_CURSOR "Cursor"

#define KEY_FB_ID "FB_ID"
#define KEY_CRTC_X "CRTC_X"
#define KEY_CRTC_Y "CRTC_Y"
#define KEY_CRTC_W "CRTC_W"
#define KEY_CRTC_H "CRTC_H"
#define KEY_SRC_X "SRC_X"
#define KEY_SRC_Y "SRC_Y"
#define KEY_SRC_W "SRC_W"
#define KEY_SRC_H "SRC_H"
#define KEY_ZPOS "ZPOS"
#define KEY_FEATURE "FEATURE"

// rknn
#define KEY_OUTPUT_WANT_FLOAT "rknn_output_want_float"
#define KEY_TENSOR_TYPE "tensor_type"
#define KEY_TENSOR_FMT "tensor_fmt"
#define KEY_NCHW "NCHW"
#define KEY_NHWC "NHWC"

#endif // #ifndef EASYMEDIA_MEDIA_KEY_STRING_H_
