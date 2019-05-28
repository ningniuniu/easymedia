# -----------------------------------------
#
# Hertz Wang wangh@rock-chips.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

set(RK_MEDIA_STREAM_SOURCE_FILES stream/file_stream.cc)
set(RK_MEDIA_STREAM_LIBS )

add_subdirectory(audio)

set(RK_MEDIA_SOURCE_FILES ${RK_MEDIA_SOURCE_FILES} ${RK_MEDIA_STREAM_SOURCE_FILES} PARENT_SCOPE)
set(RK_MEDIA_DEPENDENT_LIBS ${RK_MEDIA_DEPENDENT_LIBS} ${RK_MEDIA_STREAM_LIBS} PARENT_SCOPE)
