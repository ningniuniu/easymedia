# -----------------------------------------
#
# Hertz Wang 1989wanghang@163.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

set(EASY_MEDIA_STREAM_SOURCE_FILES stream/file_stream.cc)
set(EASY_MEDIA_STREAM_COMPILE_DEFINITIONS)
set(EASY_MEDIA_STREAM_LIBS)

add_subdirectory(audio)
add_subdirectory(camera)
add_subdirectory(display)

set(EASY_MEDIA_SOURCE_FILES ${EASY_MEDIA_SOURCE_FILES}
                            ${EASY_MEDIA_STREAM_SOURCE_FILES} PARENT_SCOPE)
set(EASY_MEDIA_COMPILE_DEFINITIONS
    ${EASY_MEDIA_STREAM_COMPILE_DEFINITIONS}
    PARENT_SCOPE)
set(EASY_MEDIA_DEPENDENT_LIBS ${EASY_MEDIA_DEPENDENT_LIBS}
                              ${EASY_MEDIA_STREAM_LIBS} PARENT_SCOPE)
