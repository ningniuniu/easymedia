# -----------------------------------------
#
# Hertz Wang wangh@rock-chips.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

project(rkmedia_rtsp_server_test)

set(RTSP_SERVER_TEST_DEPENDENT_LIBS ${RTSP_SERVER_TEST_DEPENDENT_LIBS} rkmedia)

set (CMAKE_CXX_STANDARD 11)

aux_source_directory(./ RTSP_SERVER_TEST_SRC_FILES)
add_executable(rtsp_server_test ${RTSP_SERVER_TEST_SRC_FILES})
add_dependencies(rtsp_server_test rkmedia)
target_link_libraries(rtsp_server_test ${RTSP_SERVER_TEST_DEPENDENT_LIBS})

install(TARGETS rtsp_server_test RUNTIME DESTINATION "bin")
