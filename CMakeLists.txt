cmake_minimum_required(VERSION 4.0)
project(monokakido_extractor)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MACOSX_BUNDLE TRUE)

include(FetchContent)
FetchContent_Declare(
        glaze
        GIT_REPOSITORY https://github.com/stephenberry/glaze.git
        GIT_TAG main
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glaze)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${glaze_SOURCE_DIR}/include)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/pugixml)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/utfcpp)


# Core library
add_library(monokakido_core STATIC
        src/core/platform/fs.mm
        src/core/platform/scoped_security_access.mm
)

target_link_libraries(monokakido_core PRIVATE
        "-framework Foundation"
        "-framework Security"
        "-framework CoreServices"
        "-framework Cocoa"
)

# Dictionary library
add_library(monokakido_dictionary STATIC
        src/dictionary/catalog.cpp
        src/dictionary/metadata.cpp
        src/dictionary/paths.cpp
        src/dictionary/dictionary.cpp
)


add_executable(monokakido_extractor MACOSX_BUNDLE
        src/main.cpp
)
target_link_libraries(monokakido_extractor PRIVATE
        monokakido_core
        monokakido_dictionary
        glaze::glaze
)

set_target_properties(monokakido_extractor PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "Monokakido Extractor"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.kiwaa.monokakido-extractor"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
)