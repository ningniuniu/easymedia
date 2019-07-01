# -----------------------------------------
#
# Hertz Wang 1989wanghang@163.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

option(RKRGA "compile: rkrga wrapper" OFF)
if(RKRGA)

  set(EASY_MEDIA_RKRGA_SOURCE_FILES rkrga/rga.cc)
  set(EASY_MEDIA_SOURCE_FILES ${EASY_MEDIA_SOURCE_FILES}
                              ${EASY_MEDIA_RKRGA_SOURCE_FILES} PARENT_SCOPE)
  set(EASY_MEDIA_DEPENDENT_LIBS ${EASY_MEDIA_DEPENDENT_LIBS} rga PARENT_SCOPE)

  # cmake-format: off
  # option(RKRGA_TEST "compile: rkrga wrapper test" ON)
  # if (RKRGA_TEST)
  # add_subdirectory(test)
  # endif()
  # cmake-format: on

endif()
