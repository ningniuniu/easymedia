# -----------------------------------------
#
# Hertz Wang 1989wanghang@163.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

set(EASY_MEDIA_FLOW_SOURCE_FILES flow/video_encoder_flow.cc
    flow/file_flow.cc)
# set(EASY_MEDIA_FLOW_LIBS )

set(EASY_MEDIA_SOURCE_FILES ${EASY_MEDIA_SOURCE_FILES} ${EASY_MEDIA_FLOW_SOURCE_FILES} PARENT_SCOPE)
# set(EASY_MEDIA_DEPENDENT_LIBS ${EASY_MEDIA_DEPENDENT_LIBS} ${EASY_MEDIA_FLOW_LIBS} PARENT_SCOPE)
