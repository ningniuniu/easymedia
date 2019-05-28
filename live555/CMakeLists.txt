# -----------------------------------------
#
# Hertz Wang wangh@rock-chips.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

set(RK_MEDIA_LIVE555_SOURCE_FILES )
set(RK_MEDIA_LIVE555_LIBS )

option(LIVE555_SERVER "compile: live555 server" OFF)
if (LIVE555_SERVER)
add_subdirectory(server)
endif()

set(RK_MEDIA_SOURCE_FILES ${RK_MEDIA_SOURCE_FILES} ${RK_MEDIA_LIVE555_SOURCE_FILES} PARENT_SCOPE)
set(RK_MEDIA_DEPENDENT_LIBS ${RK_MEDIA_DEPENDENT_LIBS} ${RK_MEDIA_LIVE555_LIBS} PARENT_SCOPE)
