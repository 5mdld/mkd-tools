find_package(CLI11 2.7 CONFIG QUIET)

if (NOT TARGET CLI11::CLI11)
    include(FetchContent)
    FetchContent_Declare(cli11
            GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
            GIT_TAG cbd58a3696887b34c70949aef21a71735a0c2ad5 # v2.7.2
    )
    FetchContent_MakeAvailable(cli11)
endif ()

add_library(mkd-cli-core STATIC
        src/cli.cpp
        src/source.cpp
        src/commands.cpp
        src/format.cpp
        src/progress.cpp
)

target_include_directories(mkd-cli-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(mkd-cli-core PRIVATE
        ${PROJECT_SOURCE_DIR}/third_party/indicators
        ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(mkd-cli-core
        PUBLIC MKD::mkd CLI11::CLI11
)
target_compile_definitions(mkd-cli-core PRIVATE MKD_CLI_VERSION="${PROJECT_VERSION}")

add_executable(mkd-cli src/main.cpp)
target_link_libraries(mkd-cli PRIVATE mkd-cli-core)
set_target_properties(mkd-cli PROPERTIES OUTPUT_NAME mkd-tools)

include(GNUInstallDirs)
install(TARGETS mkd-cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
