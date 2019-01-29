# ----------------------------------------------------------------------------
#
# Hertz Wang wangh@rock-chips.com
#
# SPDX-License-Identifier: Apache-2.0
#
# ----------------------------------------------------------------------------

# vi: set noexpandtab syntax=cmake:

pkg_check_modules(LIBOGG REQUIRED ogg)
pkg_check_modules(LIBVORBIS REQUIRED vorbis)

option(OGG_DECODE "compile: ogg_decode" OFF)
if (OGG_DECODE)
set(RK_MEDIA_OGG_SOURCE_FILES ogg/ogg_vorbis_demuxer.cc)
set(RK_MEDIA_OGG_LIBS vorbisfile)
endif()


set(RK_MEDIA_SOURCE_FILES ${RK_MEDIA_SOURCE_FILES} ${RK_MEDIA_OGG_SOURCE_FILES} PARENT_SCOPE)
set(RK_MEDIA_OGG_LIBS ${RK_MEDIA_OGG_LIBS} ogg vorbis)
set(RK_MEDIA_DEPENDENT_LIBS ${RK_MEDIA_DEPENDENT_LIBS} ${RK_MEDIA_OGG_LIBS} PARENT_SCOPE)

option(OGG_TEST "compile: ogg test" ON)
if(OGG_TEST)
add_subdirectory(test)
endif()
