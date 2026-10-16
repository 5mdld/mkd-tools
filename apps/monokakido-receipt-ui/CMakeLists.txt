set(APP_NAME MKDReceiptApp)
set(APP_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(${APP_NAME} MACOSX_BUNDLE
    ${APP_DIR}/AppMain.swift
    ${APP_DIR}/ReceiptView.swift
    ${APP_DIR}/ReceiptModel.swift
    ${APP_DIR}/ReceiptService.swift
    ${APP_DIR}/ReceiptCatalog.cpp
)

target_link_libraries(${APP_NAME} PRIVATE mkd
    "-framework SwiftUI"
    "-framework AppKit"
)
target_include_directories(${APP_NAME} PRIVATE ${APP_DIR})
target_compile_options(${APP_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:Swift>:-cxx-interoperability-mode=default>"
    "$<$<COMPILE_LANGUAGE:Swift>:SHELL:-Xcc -std=c++23>"
)

# Xcode passes the deployment target to Swift, but Ninja needs an explicit target triple.
if (CMAKE_OSX_DEPLOYMENT_TARGET AND NOT CMAKE_GENERATOR STREQUAL "Xcode")
    set(RECEIPT_SWIFT_ARCH ${CMAKE_SYSTEM_PROCESSOR})
    if (CMAKE_OSX_ARCHITECTURES)
        list(GET CMAKE_OSX_ARCHITECTURES 0 RECEIPT_SWIFT_ARCH)
    endif ()
    target_compile_options(${APP_NAME} PRIVATE
        "$<$<COMPILE_LANGUAGE:Swift>:SHELL:-target ${RECEIPT_SWIFT_ARCH}-apple-macosx${CMAKE_OSX_DEPLOYMENT_TARGET}>"
    )
endif ()

set_target_properties(${APP_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${APP_DIR}/Info.plist
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.kiwakiwaa.mkd.receipt"
    Swift_LANGUAGE_VERSION 6
    XCODE_ATTRIBUTE_SWIFT_VERSION 6.0
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.kiwakiwaa.mkd.receipt"
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO
    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
)
