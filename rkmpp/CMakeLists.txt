# -----------------------------------------
#
# Hertz Wang 1989wanghang@163.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

pkg_check_modules(ROCKCHIP_MPP REQUIRED rockchip_mpp)

set(EASY_MEDIA_SOURCE_FILES ${EASY_MEDIA_SOURCE_FILES}
							rkmpp/mpp_encoder.cc
							rkmpp/mpp_inc.cc
							rkmpp/mpp_h264_encoder.cc
							rkmpp/mpp_jpeg_encoder.cc
							PARENT_SCOPE)

set(EASY_MEDIA_DEPENDENT_LIBS ${EASY_MEDIA_DEPENDENT_LIBS} mpp PARENT_SCOPE)

option(RKMPP_TEST "compile: rkmpp wrapper test" ON)
if( RKMPP_TEST )
add_subdirectory(test)
endif()
