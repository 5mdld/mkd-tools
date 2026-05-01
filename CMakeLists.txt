cmake_minimum_required(VERSION 3.25)
project(mkd-tools LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (POLICY CMP0069)
    cmake_policy(SET CMP0069 NEW)
endif ()

if (APPLE)
    enable_language(OBJCXX)
endif ()

option(BUILD_TESTING "Build tests" OFF)
option(MKD_BUILD_CLI "Build command-line interface" ON)
option(MKD_BUILD_RECEIPT_APP "Build macOS receipt app" OFF)

if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported)

    if (ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif ()
endif ()

add_library(mkd)
add_library(MKD::mkd ALIAS mkd)

target_compile_features(mkd PUBLIC cxx_std_23)
target_include_directories(mkd PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_include_directories(mkd PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    if (MSVC)
        target_compile_definitions(mkd PRIVATE NOMINMAX)
    else ()
        if (CMAKE_CROSSCOMPILING)
            target_compile_options(mkd PRIVATE -O3)
        else ()
            target_compile_options(mkd PRIVATE -march=native -O3)
        endif ()

        if (APPLE)
            target_link_options(mkd PRIVATE -Wl,-dead_strip)
        elseif (UNIX)
            target_compile_options(mkd PRIVATE -ffunction-sections -fdata-sections)
            target_link_options(mkd PRIVATE -s -Wl,--gc-sections -Wl,--exclude-libs,ALL)
        endif ()
    endif ()
endif ()

add_subdirectory(third_party)
add_subdirectory(src)

if (MKD_BUILD_CLI)
    add_subdirectory(cli)
endif ()

if (MKD_BUILD_RECEIPT_APP)
    add_subdirectory(apps)
endif ()

if (BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif ()
