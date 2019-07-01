# -----------------------------------------
#
# Hertz Wang 1989wanghang@163.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

option(LIVE555_SERVER_H264 "compile: live555 server with h264" OFF)
if(LIVE555_SERVER_H264)
  set(EASY_MEDIA_LIVE555_SERVER_SOURCE_FILES
      live555/server/h264_server_media_subsession.cc)
endif()

set(EASY_MEDIA_LIVE555_SERVER_SOURCE_FILES
    ${EASY_MEDIA_LIVE555_SERVER_SOURCE_FILES}
    live555/server/live555_media_input.cc live555/server/rtsp_server.cc)

set(EASY_MEDIA_LIVE555_SOURCE_FILES
    ${EASY_MEDIA_LIVE555_SOURCE_FILES} ${EASY_MEDIA_LIVE555_SERVER_SOURCE_FILES}
    PARENT_SCOPE)
set(EASY_MEDIA_LIVE555_LIBS
    ${EASY_MEDIA_LIVE555_LIBS}
    BasicUsageEnvironment
    groupsock
    liveMedia
    UsageEnvironment
    PARENT_SCOPE)
option(RTSP_SERVER_TEST "compile: rtsp server test" ON)
if(RTSP_SERVER_TEST)
  add_subdirectory(test)
endif()
