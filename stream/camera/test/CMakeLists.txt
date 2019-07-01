# -----------------------------------------
#
# Hertz Wang 1989wanghang@163.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

project(easymedia_camera_test)

set(CAMERA_TEST_DEPENDENT_LIBS easymedia)

set(CMAKE_CXX_STANDARD 11)

add_definitions(-DDEBUG)

if(V4L2_CAPTURE)
  set(CAMERA_CAP_TEST_SRC_FILES camera_capture_test.cc)
  add_executable(camera_cap_test ${CAMERA_CAP_TEST_SRC_FILES})
  add_dependencies(camera_cap_test easymedia)
  target_link_libraries(camera_cap_test ${CAMERA_TEST_DEPENDENT_LIBS})
  install(TARGETS camera_cap_test RUNTIME DESTINATION "bin")
endif()
