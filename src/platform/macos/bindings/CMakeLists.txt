set(SWIFT_BRIDGE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/CryptoKitBridge.swift")
set(SWIFT_BRIDGE_OBJ "${CMAKE_CURRENT_BINARY_DIR}/CryptoKitBridge.o")

find_program(SWIFTC_EXECUTABLE swiftc REQUIRED)

add_custom_command(
        OUTPUT ${SWIFT_BRIDGE_OBJ}
        COMMAND ${SWIFTC_EXECUTABLE}
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
