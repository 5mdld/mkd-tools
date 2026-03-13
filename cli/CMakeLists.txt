cmake_minimum_required(VERSION 3.25)

# executable
add_executable(mkd-cli
        src/main.cpp
        src/cli.cpp
        src/commands.cpp
        src/format.cpp
        src/progress.cpp
)

target_include_directories(mkd-cli PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/third_party/indicators
)

target_link_libraries(mkd-cli PRIVATE mkd)
target_compile_features(mkd-cli PRIVATE cxx_std_23)
set_target_properties(mkd-cli PROPERTIES
        OUTPUT_NAME mkd-tools
)

include(GNUInstallDirs)
install(TARGETS mkd-cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
