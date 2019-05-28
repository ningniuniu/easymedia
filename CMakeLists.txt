# -----------------------------------------
#
# Hertz Wang wangh@rock-chips.com
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# -----------------------------------------

# vi: set noexpandtab syntax=cmake:

message(STATUS "cmake version ${CMAKE_VERSION}")
if(NOT CMAKE_VERSION VERSION_LESS "2.8.12.20131121")
	cmake_policy(SET CMP0025 OLD) # report Apple's Clang as just Clang
	cmake_policy(SET CMP0042 OLD) # do not require MACOSX_RPATH
endif()

# Search packages for host system instead of packages for target system
# in case of cross compilation these macro should be defined by toolchain file
if(NOT COMMAND find_host_package)
	macro(find_host_package)
		find_package(${ARGN})
	endmacro()
endif()
if(NOT COMMAND find_host_program)
	macro(find_host_program)
		find_program(${ARGN})
	endmacro()
endif()

project (rkmedia)

cmake_minimum_required (VERSION 3.1.0 FATAL_ERROR)
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckCXXCompilerFlag)

# ----------------------------------------------------------------------------
# set property to classify library kinds
# ----------------------------------------------------------------------------
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")
# ----------------------------------------------------------------------------
# enable test in this project
# ----------------------------------------------------------------------------
enable_testing()

# ----------------------------------------------------------------------------
# Compiler detection
# ----------------------------------------------------------------------------
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CLANG true)
endif()
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(GCC true)
endif()

if(CLANG)
	# treat clang roughly like gcc
	set(GCC true)
	add_definitions(-Wall -Wextra -Wshadow -ffast-math)
elseif(CMAKE_COMPILER_IS_GNUCXX)
	add_definitions(-Wall -Wextra -Wshadow -ffast-math)
	check_cxx_compiler_flag(-Wno-narrowing GCC_HAS_NO_NARROWING)
	check_cxx_compiler_flag(-mstackrealign GCC_HAS_STACK_REALIGN)
	if (GCC_HAS_STACK_REALIGN)
		add_definitions(-mstackrealign)
	endif()
	execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
endif()

if($(CMAKE_BUILD_TYPE) MATCHES "Release")
	set(CMAKE_C_VISIBILITY_PRESET hidden)
	set(CMAKE_CXX_VISIBILITY_PRESET hidden)
endif()

# set c++11
set (CMAKE_CXX_STANDARD 11)
# rtti may be expensive on Embedded Platform, obey Google C++ Style Guide
add_definitions(-fno-rtti)

# add PIC flag
add_definitions(-fPIC)
#set (CMAKE_SHARED_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})

include_directories(./)

# ----------------------------------------------------------------------------
# Build options
# ----------------------------------------------------------------------------
find_package(PkgConfig)
INCLUDE(GNUInstallDirs)

# ----------------------------------------------------------------------------
# Set Warning as Error
# ----------------------------------------------------------------------------
option(WARNINGS_AS_ERRORS "Stop compiles on first warning" OFF)
if(WARNINGS_AS_ERRORS)
	if(GCC)
		add_definitions(-Werror)
	elseif(MSVC)
		add_definitions(/WX)
	endif()
endif(WARNINGS_AS_ERRORS)

option(FLOW "compile: flow" ON)
option(STREAM "compile: stream" ON)
option(ENCODER "compile: encoder" ON)
option(DECODER "compile: decoder" ON)
option(MUXER "compile: muxer" ON)
option(DEMUXER "compile: demuxer" ON)

pkg_check_modules(LIBDRM libdrm)
if ( LIBDRM_FOUND )
add_definitions(-DLIBDRM)
include_directories(${LIBDRM_INCLUDE_DIRS})
set(RK_MEDIA_DEPENDENT_LIBS drm)
endif()

pkg_check_modules(LIBION libion)
if (LIBION_FOUND)
add_definitions(-DLIBION)
include_directories(${LIBION_INCLUDE_DIRS})
set(RK_MEDIA_DEPENDENT_LIBS ion)
endif()

aux_source_directory(. RK_MEDIA_SOURCE_FILES)

# ----------------------------------------------------------------------------
#  Start module definition
# ----------------------------------------------------------------------------

option(RKMPP "compile: rkmpp wrapper" OFF)
if( RKMPP )
include_directories(rkmpp)
add_subdirectory(rkmpp)
endif()

option(OGG "compile: ogg" OFF)
if ( OGG )
add_subdirectory(ogg)
endif()

option(LIVE555 "compile: live555" OFF)
if ( LIVE555 )
include_directories(${CMAKE_INSTALL_PREFIX}/include/groupsock)
include_directories(${CMAKE_INSTALL_PREFIX}/include/UsageEnvironment)
add_subdirectory(live555)
endif()

add_subdirectory(stream)
add_subdirectory(flow)

add_library(rkmedia SHARED ${RK_MEDIA_SOURCE_FILES})
file(GLOB RK_MEDIA_RELEASE_HEADERS *.h)
set_target_properties(rkmedia PROPERTIES
					  PUBLIC_HEADER "${RK_MEDIA_RELEASE_HEADERS}")

if( RK_MEDIA_DEPENDENT_LIBS )
target_link_libraries(rkmedia ${RK_MEDIA_DEPENDENT_LIBS})
endif()

# message(headers: "${RK_MEDIA_RELEASE_HEADERS}")
# message(files: "${RK_MEDIA_SOURCE_FILES}")
# message(libs : "${RK_MEDIA_DEPENDENT_LIBS}")

install(TARGETS rkmedia
		LIBRARY DESTINATION "lib"
		PUBLIC_HEADER DESTINATION "include/rkmedia")
