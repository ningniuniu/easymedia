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

aux_source_directory(./ RKMPP_TEST_SRC_FILES)
add_executable(rkmpp_test ${RKMPP_TEST_SRC_FILES})
add_dependencies(rkmpp_test easymedia)
target_link_libraries(rkmpp_test ${RKMPP_TEST_DEPENDENT_LIBS})

install(TARGETS rkmpp_test RUNTIME DESTINATION "bin")
