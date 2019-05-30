# -----------------------------------------
#
# Hertz Wang 1989wanghang@163.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

project(easymedia_rkmpp_test)

pkg_check_modules(LIBDRM libdrm)
if ( LIBDRM_FOUND )
add_definitions(-DLIBDRM)
include_directories(${LIBDRM_INCLUDE_DIRS})
set(RKMPP_TEST_DEPENDENT_LIBS drm)
else()
add_definitions(-DLIBION)
set(RKMPP_TEST_DEPENDENT_LIBS ion)
endif()

set(RKMPP_TEST_DEPENDENT_LIBS ${RKMPP_TEST_DEPENDENT_LIBS} easymedia)

set (CMAKE_CXX_STANDARD 11)

if( RKMPP_ENCODER )
set(RKMPP_ENC_TEST_SRC_FILES mpp_enc_test.cc test_utils.cc)
add_executable(rkmpp_enc_test ${RKMPP_ENC_TEST_SRC_FILES})
add_dependencies(rkmpp_enc_test easymedia)
target_link_libraries(rkmpp_enc_test ${RKMPP_TEST_DEPENDENT_LIBS})
install(TARGETS rkmpp_enc_test RUNTIME DESTINATION "bin")
endif()

if( RKMPP_DECODER )
set(RKMPP_DEC_TEST_SRC_FILES mpp_dec_test.cc)
add_executable(rkmpp_dec_test ${RKMPP_DEC_TEST_SRC_FILES})
add_dependencies(rkmpp_dec_test easymedia)
target_link_libraries(rkmpp_dec_test ${RKMPP_TEST_DEPENDENT_LIBS} pthread)
install(TARGETS rkmpp_dec_test RUNTIME DESTINATION "bin")
endif()
