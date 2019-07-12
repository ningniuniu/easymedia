# -----------------------------------------
#
# Hertz Wang 1989wanghang@163.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

set(EASY_MEDIA_FLOW_SOURCE_FILES
    flow/video_encoder_flow.cc
    flow/decoder_flow.cc
    flow/file_flow.cc
    flow/filter_flow.cc
    flow/source_stream_flow.cc
    flow/output_stream_flow.cc)

set(EASY_MEDIA_SOURCE_FILES ${EASY_MEDIA_SOURCE_FILES}
                            ${EASY_MEDIA_FLOW_SOURCE_FILES} PARENT_SCOPE)
