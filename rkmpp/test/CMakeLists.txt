project(rkmedia_rkmpp_test)

pkg_check_modules(LIBDRM libdrm)
if ( LIBDRM_FOUND )
add_definitions(-DLIBDRM)
include_directories(${LIBDRM_INCLUDE_DIRS})
set(RKMPP_TEST_DEPENDENT_LIBS drm)
else()
add_definitions(-DLIBION)
set(RKMPP_TEST_DEPENDENT_LIBS ion)
endif()

set(RKMPP_TEST_DEPENDENT_LIBS ${RKMPP_TEST_DEPENDENT_LIBS} rkmedia)


set (CMAKE_CXX_STANDARD 11)

aux_source_directory(./ RKMPP_TEST_SRC_FILES)
add_executable(rkmpp_test ${RKMPP_TEST_SRC_FILES})
add_dependencies(rkmpp_test rkmedia)
target_link_libraries(rkmpp_test ${RKMPP_TEST_DEPENDENT_LIBS})

install(TARGETS rkmpp_test RUNTIME DESTINATION "bin")
