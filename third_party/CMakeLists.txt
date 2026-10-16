set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

foreach (dependency IN ITEMS glaze utf8proc pugixml cli11 indicators googletest zlib)
    if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dependency}/CMakeLists.txt")
        message(FATAL_ERROR "Missing ${dependency} submodule. Run: git submodule update --init --recursive")
    endif ()
endforeach ()

add_subdirectory(glaze EXCLUDE_FROM_ALL)

set(UTF8PROC_INSTALL OFF)
set(UTF8PROC_ENABLE_TESTING OFF)
add_subdirectory(utf8proc EXCLUDE_FROM_ALL)
add_library(utf8proc::utf8proc ALIAS utf8proc)

set(PUGIXML_BUILD_TESTS OFF)
set(PUGIXML_INSTALL OFF)
add_subdirectory(pugixml EXCLUDE_FROM_ALL)
target_link_libraries(mkd PRIVATE pugixml::pugixml)

find_package(ZLIB QUIET GLOBAL)
if (NOT TARGET ZLIB::ZLIB)
    set(ZLIB_BUILD_EXAMPLES OFF)
    # zlib 1.3.1 renames a source header during configuration. keep that in the build tree.
    file(COPY zlib/ DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/zlib-source" PATTERN ".git" EXCLUDE)
    add_subdirectory("${CMAKE_CURRENT_BINARY_DIR}/zlib-source" zlib EXCLUDE_FROM_ALL)
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
endif ()

if (MKD_BUILD_CLI)
    add_subdirectory(cli11 EXCLUDE_FROM_ALL)

    set(INDICATORS_BUILD_TESTS OFF)
    set(INDICATORS_SAMPLES OFF)
    set(INDICATORS_DEMO OFF)
    add_subdirectory(indicators EXCLUDE_FROM_ALL)
endif ()

if (BUILD_TESTING)
    set(BUILD_GMOCK OFF)
    set(INSTALL_GTEST OFF)
    set(gtest_force_shared_crt ON)
    add_subdirectory(googletest EXCLUDE_FROM_ALL)
endif ()
