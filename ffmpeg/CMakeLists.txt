# -----------------------------------------
#
# Hertz Wang 1989wanghang@163.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

option(FFMPEG "compile: ffmpeg wrapper" OFF)
if(FFMPEG)
  pkg_check_modules(LIBAVFORMAT REQUIRED libavformat>=58.20.100)
  set(EASY_MEDIA_FFMPEG_SOURCE_FILES ffmpeg/ffmpeg_utils.cc)
  if(MUXER)
    set(EASY_MEDIA_FFMPEG_SOURCE_FILES ${EASY_MEDIA_FFMPEG_SOURCE_FILES}
                                       ffmpeg/ffmpeg_muxer.cc)
  endif()
  if(ENCODER)
    set(EASY_MEDIA_FFMPEG_SOURCE_FILES ${EASY_MEDIA_FFMPEG_SOURCE_FILES}
                                       ffmpeg/ffmpeg_aud_encoder.cc)
  endif()

  set(EASY_MEDIA_SOURCE_FILES ${EASY_MEDIA_SOURCE_FILES}
                              ${EASY_MEDIA_FFMPEG_SOURCE_FILES} PARENT_SCOPE)
  set(EASY_MEDIA_DEPENDENT_LIBS
      ${EASY_MEDIA_DEPENDENT_LIBS}
      avformat
      avcodec
      avutil
      PARENT_SCOPE)

# cmake-format: off
option(FFMPEG_TEST "compile: ffmpeg wrapper test" ON)
if(FFMPEG_TEST)
  add_subdirectory(test)
endif()
# cmake-format: on

endif()
