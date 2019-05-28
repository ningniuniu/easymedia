# -----------------------------------------
#
# Hertz Wang wangh@rock-chips.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

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

option(OGG_ENCODE_TEST "compile: ogg encode test" ON)
if (OGG_ENCODE_TEST)
set(ENC_TEST_SRC_FILES ogg_encode_test.cc)
add_executable(rkogg_encode_test ${ENC_TEST_SRC_FILES})
add_dependencies(rkogg_encode_test rkmedia)
target_link_libraries(rkogg_encode_test rkmedia)
install(TARGETS rkogg_encode_test RUNTIME DESTINATION "bin")
endif()
