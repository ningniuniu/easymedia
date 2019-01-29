# Hertz Wang wangh@rock-chips.com
#
# SPDX-License-Identifier: Apache-2.0

# vi: set noexpandtab syntax=cmake:

project(rkmedia_ogg_test)

set (CMAKE_CXX_STANDARD 11)

option(OGG_DECODE_TEST "compile: ogg decode test" ON)
if (OGG_DECODE_TEST)
set(DEC_TEST_SRC_FILES ogg_decode_test.cc)
add_executable(rkogg_decode_test ${DEC_TEST_SRC_FILES})
add_dependencies(rkogg_decode_test rkmedia)
target_link_libraries(rkogg_decode_test rkmedia)
install(TARGETS rkogg_decode_test RUNTIME DESTINATION "bin")
endif()
