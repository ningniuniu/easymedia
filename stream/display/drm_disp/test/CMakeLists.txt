# -----------------------------------------
#
# Hertz Wang 1989wanghang@163.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

project(easymedia_drm_display_test)

set(DRM_DISPLAY_TEST_DEPENDENT_LIBS easymedia)

set(CMAKE_CXX_STANDARD 11)

add_definitions(-DDEBUG)

set(DRM_DISPLAY_TEST_SRC_FILES drm_display_test.cc)
add_executable(drm_display_test ${DRM_DISPLAY_TEST_SRC_FILES})
add_dependencies(drm_display_test easymedia)
target_link_libraries(drm_display_test ${DRM_DISPLAY_TEST_DEPENDENT_LIBS})
install(TARGETS drm_display_test RUNTIME DESTINATION "bin")
