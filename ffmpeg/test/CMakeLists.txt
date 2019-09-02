# -----------------------------------------
#
# Hertz Wang 1989wanghang@163.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

project(easymedia_ffmpeg_test)

set(FFMPEG_TEST_DEPENDENT_LIBS easymedia)

set(CMAKE_CXX_STANDARD 11)

add_definitions(-DDEBUG)

add_executable(ffmpeg_enc_mux_test ffmpeg_enc_mux_test.cc)
add_dependencies(ffmpeg_enc_mux_test easymedia)
target_link_libraries(ffmpeg_enc_mux_test ${FFMPEG_TEST_DEPENDENT_LIBS})
install(TARGETS ffmpeg_enc_mux_test RUNTIME DESTINATION "bin")
