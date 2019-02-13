# ----------------------------------------------------------------------------
#
# Hertz Wang wangh@rock-chips.com
#
# SPDX-License-Identifier: Apache-2.0
#
# ----------------------------------------------------------------------------

# vi: set noexpandtab syntax=cmake:

option(ALSA_PLAYBACK "compile: alsa playback" OFF)
option(ALSA_CAPTURE "compile: alsa capture" OFF)

if (ALSA_PLAYBACK OR ALSA_CAPTURE)
pkg_check_modules(ALSA REQUIRED alsa)
set(RK_MEDIA_STREAM_LIBS asound)
set(RK_MEDIA_STREAM_SOURCE_FILES stream/alsa_utils.cc)

if (ALSA_PLAYBACK)
set(RK_MEDIA_STREAM_SOURCE_FILES ${RK_MEDIA_STREAM_SOURCE_FILES} stream/alsa_playback_stream.cc)
endif()

endif()

set(RK_MEDIA_SOURCE_FILES ${RK_MEDIA_SOURCE_FILES} ${RK_MEDIA_STREAM_SOURCE_FILES} PARENT_SCOPE)
set(RK_MEDIA_DEPENDENT_LIBS ${RK_MEDIA_DEPENDENT_LIBS} ${RK_MEDIA_STREAM_LIBS} PARENT_SCOPE)

# playback test ref to ogg/test/ogg_decode_test.cc
