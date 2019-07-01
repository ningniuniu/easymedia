# -----------------------------------------
#
# Hertz Wang 1989wanghang@163.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

option(DRM_DISPLAY "compile: drm display" OFF)
if(DRM_DISPLAY)
  set(EASY_MEDIA_STREAM_DISPLAY_SOURCE_FILES
      stream/display/drm_disp/drm_utils.cc stream/display/drm_disp/drm_stream.cc
      stream/display/drm_disp/drm_output_stream.cc
      PARENT_SCOPE)
  add_subdirectory(test)
endif()
