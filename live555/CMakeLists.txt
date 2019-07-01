# -----------------------------------------
#
# Hertz Wang 1989wanghang@163.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

set(EASY_MEDIA_LIVE555_SOURCE_FILES)
set(EASY_MEDIA_LIVE555_LIBS)

option(LIVE555_SERVER "compile: live555 server" OFF)
if(LIVE555_SERVER)
  add_subdirectory(server)
endif()

set(EASY_MEDIA_SOURCE_FILES ${EASY_MEDIA_SOURCE_FILES}
                            ${EASY_MEDIA_LIVE555_SOURCE_FILES} PARENT_SCOPE)
set(EASY_MEDIA_DEPENDENT_LIBS ${EASY_MEDIA_DEPENDENT_LIBS}
                              ${EASY_MEDIA_LIVE555_LIBS} PARENT_SCOPE)
