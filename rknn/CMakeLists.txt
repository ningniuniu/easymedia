# -----------------------------------------
#
# Hertz Wang 1989wanghang@163.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

option(RKNN "compile: rknn wrapper" OFF)
if(RKNN)
  set(EASY_MEDIA_RKNN_SOURCE_FILES rknn/rknn.cc)
  set(EASY_MEDIA_SOURCE_FILES ${EASY_MEDIA_SOURCE_FILES}
                              ${EASY_MEDIA_RKNN_SOURCE_FILES} PARENT_SCOPE)
  set(EASY_MEDIA_DEPENDENT_LIBS
      ${EASY_MEDIA_DEPENDENT_LIBS} rknn_runtime
      PARENT_SCOPE)
endif()
