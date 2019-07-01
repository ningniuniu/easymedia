# -----------------------------------------
#
# Hertz Wang 1989wanghang@163.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

option(OGGVORBIS "" OFF)
if(OGGVORBIS)
  pkg_check_modules(LIBOGG REQUIRED ogg)
  pkg_check_modules(LIBVORBIS REQUIRED vorbis)
endif()

option(OGGVORBIS_DEMUXER "compile: oggvorbis_demuxer" OFF)
if(OGGVORBIS_DEMUXER)
  set(EASY_MEDIA_OGG_SOURCE_FILES ogg/ogg_vorbis_demuxer.cc)
  set(EASY_MEDIA_OGG_LIBS vorbisfile)
endif(OGGVORBIS_DEMUXER)

option(OGGVORBIS_MUXER "" OFF)
if(OGGVORBIS_MUXER)
  option(VORBIS_ENCODER "compile: libvorbis encoder" ON)
  if(VORBIS_ENCODER)
    set(EASY_MEDIA_OGG_SOURCE_FILES ${EASY_MEDIA_OGG_SOURCE_FILES}
                                    ogg/vorbis_encoder.cc)
    set(EASY_MEDIA_OGG_LIBS ${EASY_MEDIA_OGG_LIBS} vorbisenc)
  endif()

  option(OGG_MUXER "compile: libogg muxer" ON)
  if(OGG_MUXER)
    set(EASY_MEDIA_OGG_SOURCE_FILES ${EASY_MEDIA_OGG_SOURCE_FILES}
                                    ogg/ogg_muxer.cc)
    set(EASY_MEDIA_OGG_LIBS ${EASY_MEDIA_OGG_LIBS} ogg)
  endif()

  if(VORBIS_ENCODER OR OGG_MUXER)
    set(EASY_MEDIA_OGG_SOURCE_FILES ${EASY_MEDIA_OGG_SOURCE_FILES}
                                    ogg/ogg_utils.cc)
  endif()
endif(OGGVORBIS_MUXER)

set(EASY_MEDIA_SOURCE_FILES ${EASY_MEDIA_SOURCE_FILES}
                            ${EASY_MEDIA_OGG_SOURCE_FILES} PARENT_SCOPE)
set(EASY_MEDIA_OGG_LIBS ${EASY_MEDIA_OGG_LIBS} ogg vorbis)
set(EASY_MEDIA_DEPENDENT_LIBS ${EASY_MEDIA_DEPENDENT_LIBS}
                              ${EASY_MEDIA_OGG_LIBS} PARENT_SCOPE)

option(OGG_TEST "compile: ogg test" ON)
if(OGG_TEST)
  add_subdirectory(test)
endif()
