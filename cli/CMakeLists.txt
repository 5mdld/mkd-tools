add_library(mkd-cli-core STATIC
        src/cli.cpp
        src/source.cpp
        src/commands.cpp
        src/format.cpp
        src/progress.cpp
)

if (APPLE)
    target_sources(mkd-cli-core PRIVATE src/macos/install.cpp src/macos/install_transaction.cpp)
    target_link_libraries(mkd-cli-core PRIVATE pugixml::pugixml)
endif ()

target_include_directories(mkd-cli-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(mkd-cli-core PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(mkd-cli-core
        PUBLIC MKD::mkd CLI11::CLI11 indicators::indicators
)
target_compile_definitions(mkd-cli-core PRIVATE MKD_CLI_VERSION="${PROJECT_VERSION}")

add_executable(mkd-cli src/main.cpp)
target_link_libraries(mkd-cli PRIVATE mkd-cli-core)
set_target_properties(mkd-cli PROPERTIES OUTPUT_NAME mkd-tools)

include(GNUInstallDirs)
install(TARGETS mkd-cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
