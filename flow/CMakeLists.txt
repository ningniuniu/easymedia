# -----------------------------------------
#
# Hertz Wang wangh@rock-chips.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

set(RK_MEDIA_FLOW_SOURCE_FILES flow/video_encoder_flow.cc
    flow/file_flow.cc)
# set(RK_MEDIA_FLOW_LIBS )

set(RK_MEDIA_SOURCE_FILES ${RK_MEDIA_SOURCE_FILES} ${RK_MEDIA_FLOW_SOURCE_FILES} PARENT_SCOPE)
# set(RK_MEDIA_DEPENDENT_LIBS ${RK_MEDIA_DEPENDENT_LIBS} ${RK_MEDIA_FLOW_LIBS} PARENT_SCOPE)
