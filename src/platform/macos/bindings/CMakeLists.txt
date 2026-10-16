set(SWIFT_BRIDGE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/CryptoKitBridge.swift")
set(SWIFT_BRIDGE_OBJ "${CMAKE_CURRENT_BINARY_DIR}/CryptoKitBridge.o")

find_program(SWIFTC_EXECUTABLE swiftc REQUIRED)

set(CRYPTOKIT_SWIFT_FLAGS)
if (CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CRYPTOKIT_SWIFT_ARCH ${CMAKE_SYSTEM_PROCESSOR})
    if (CMAKE_OSX_ARCHITECTURES)
        list(GET CMAKE_OSX_ARCHITECTURES 0 CRYPTOKIT_SWIFT_ARCH)
    endif ()
    list(APPEND CRYPTOKIT_SWIFT_FLAGS
            -target "${CRYPTOKIT_SWIFT_ARCH}-apple-macosx${CMAKE_OSX_DEPLOYMENT_TARGET}")
endif ()

add_custom_command(
        OUTPUT ${SWIFT_BRIDGE_OBJ}
        COMMAND ${SWIFTC_EXECUTABLE}
        ${CRYPTOKIT_SWIFT_FLAGS}
        -parse-as-library
        -module-name CryptoKitBridge
        -whole-module-optimization
        -c ${SWIFT_BRIDGE_SRC}
        -o ${SWIFT_BRIDGE_OBJ}
        DEPENDS ${SWIFT_BRIDGE_SRC}
        COMMENT "Compiling CryptoKit Swift bridge"
)

add_library(CRYPTOKIT_CPP
        definitions.cpp
        ${SWIFT_BRIDGE_OBJ}
)

target_include_directories(CRYPTOKIT_CPP PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(CRYPTOKIT_CPP PUBLIC
        "-framework CryptoKit"
        "-framework Foundation"
        "-L$ENV{SDKROOT}/usr/lib/swift"
        "-L/usr/lib/swift"
        "-lswiftCore"
)
