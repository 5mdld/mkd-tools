file(GLOB_RECURSE MKD_TEST_SOURCES CONFIGURE_DEPENDS
        "*.cpp"
)

list(FILTER MKD_TEST_SOURCES EXCLUDE REGEX "/crypto/test_chacha_poly\\.cpp$")
list(FILTER MKD_TEST_SOURCES EXCLUDE REGEX "/platform/macos/test_cpp_bindings\\.cpp$")

list(FILTER MKD_TEST_SOURCES EXCLUDE REGEX "/resource/resource_test\\.cpp$")

add_executable(mkd-tests
        ${MKD_TEST_SOURCES}
)

target_include_directories(mkd-tests PRIVATE
        "${PROJECT_SOURCE_DIR}/src"
)

target_link_libraries(mkd-tests PRIVATE
        mkd
        GTest::gtest
        pugixml::pugixml
)

include(GoogleTest)
gtest_discover_tests(mkd-tests DISCOVERY_TIMEOUT 300)

if (APPLE)
    add_executable(cryptokit-tests
            crypto/test_chacha_poly.cpp
    )

    target_link_libraries(cryptokit-tests PRIVATE
            mkd-macos-bindings
            GTest::gtest
            GTest::gtest_main
    )

    gtest_discover_tests(cryptokit-tests DISCOVERY_TIMEOUT 300)
endif ()

add_executable(mkd-resource-tests resource/resource_test.cpp)
target_include_directories(mkd-resource-tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(mkd-resource-tests PRIVATE mkd GTest::gtest_main)
gtest_discover_tests(mkd-resource-tests DISCOVERY_TIMEOUT 300)
