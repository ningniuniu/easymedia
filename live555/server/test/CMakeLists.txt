# -----------------------------------------
#
# Hertz Wang 1989wanghang@163.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

project(easymedia_rtsp_server_test)

set(RTSP_SERVER_TEST_DEPENDENT_LIBS ${RTSP_SERVER_TEST_DEPENDENT_LIBS}
                                    easymedia)

set(CMAKE_CXX_STANDARD 11)

add_definitions(-DDEBUG)

aux_source_directory(./ RTSP_SERVER_TEST_SRC_FILES)
add_executable(rtsp_server_test ${RTSP_SERVER_TEST_SRC_FILES})
add_dependencies(rtsp_server_test easymedia)
target_link_libraries(rtsp_server_test ${RTSP_SERVER_TEST_DEPENDENT_LIBS})

install(TARGETS rtsp_server_test RUNTIME DESTINATION "bin")
