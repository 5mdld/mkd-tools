set(CMAKE_POLICY_DEFAULT_CMP0000 OLD)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

include(FetchContent)

FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE MKD_TEST_SOURCES CONFIGURE_DEPENDS
        "*.cpp"
)

list(FILTER MKD_TEST_SOURCES EXCLUDE REGEX "/crypto/test_chacha_poly\\.cpp$")
list(FILTER MKD_TEST_SOURCES EXCLUDE REGEX "/platform/macos/test_cpp_bindings\\.cpp$")

add_executable(mkd-tests
        ${MKD_TEST_SOURCES}
)

target_include_directories(mkd-tests PRIVATE
        "${PROJECT_SOURCE_DIR}/src"
        "${PROJECT_SOURCE_DIR}/third_party/pugixml"
)

target_link_libraries(mkd-tests PRIVATE
        mkd
        GTest::gtest
)

include(GoogleTest)
gtest_discover_tests(mkd-tests DISCOVERY_TIMEOUT 300)

if (APPLE)
    add_executable(cryptokit-tests
            crypto/test_chacha_poly.cpp
    )

    target_link_libraries(cryptokit-tests PRIVATE
            CRYPTOKIT_CPP
            GTest::gtest
            GTest::gtest_main
    )

    gtest_discover_tests(cryptokit-tests DISCOVERY_TIMEOUT 300)
endif ()
