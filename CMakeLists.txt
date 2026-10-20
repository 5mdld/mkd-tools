cmake_minimum_required(VERSION 3.31)
project(mkd-tools VERSION 0.10.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTING "Build tests" OFF)
option(MKD_BUILD_PYTHON "Build Python bindings" OFF)
option(MKD_BUILD_CLI "Build command-line interface" ON)

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

if (WIN32)
    target_compile_definitions(mkd PRIVATE NOMINMAX)
endif ()

add_subdirectory(third_party)
add_subdirectory(src)

if (MKD_BUILD_CLI)
    add_subdirectory(cli)
endif ()

if (MKD_BUILD_PYTHON OR BUILD_TESTING)
    find_package(Python 3.10 REQUIRED COMPONENTS Interpreter Development.Module)
    add_subdirectory(python)
endif ()

if (BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif ()
