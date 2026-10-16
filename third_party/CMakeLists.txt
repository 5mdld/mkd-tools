include(FetchContent)

set(CMAKE_FIND_PACKAGE_TARGETS_GLOBAL TRUE)

find_package(glaze CONFIG QUIET)

if (NOT TARGET glaze::glaze)
    FetchContent_Declare(glaze
            GIT_REPOSITORY https://github.com/stephenberry/glaze.git
            GIT_TAG main
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(glaze)
endif ()

if (NOT TARGET glaze::glaze AND TARGET glaze)
    add_library(glaze::glaze ALIAS glaze)
endif ()

find_package(ZLIB QUIET)

if (NOT TARGET ZLIB::ZLIB)
    FetchContent_Declare(zlib
            GIT_REPOSITORY https://github.com/madler/zlib.git
            GIT_TAG v1.3.1
            GIT_SHALLOW TRUE
    )

    set(ZLIB_BUILD_EXAMPLES OFF)
    FetchContent_MakeAvailable(zlib)

    if (TARGET zlibstatic)
        add_library(ZLIB::ZLIB ALIAS zlibstatic)
    elseif (TARGET zlib)
        add_library(ZLIB::ZLIB ALIAS zlib)
    else ()
        message(FATAL_ERROR "No zlib target was created")
    endif ()
endif ()

find_package(utf8proc CONFIG QUIET)

if (NOT TARGET utf8proc::utf8proc)
    find_package(PkgConfig QUIET)

    if (PkgConfig_FOUND)
        pkg_check_modules(UTF8PROC QUIET IMPORTED_TARGET GLOBAL libutf8proc)
    endif ()

    if (TARGET PkgConfig::UTF8PROC)
        add_library(utf8proc::utf8proc ALIAS PkgConfig::UTF8PROC)
    endif ()
endif ()

if (NOT TARGET utf8proc::utf8proc)
    FetchContent_Declare(utf8proc
            GIT_REPOSITORY https://github.com/JuliaStrings/utf8proc.git
            GIT_TAG v2.10.0
            GIT_SHALLOW TRUE
    )

    set(UTF8PROC_INSTALL OFF CACHE BOOL "" FORCE)
    set(UTF8PROC_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(utf8proc)
endif ()

if (NOT TARGET utf8proc::utf8proc AND TARGET utf8proc)
    add_library(utf8proc::utf8proc ALIAS utf8proc)
endif ()

target_sources(mkd PRIVATE
        pugixml/pugixml.cpp
)

target_include_directories(mkd PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/pugixml"
)
