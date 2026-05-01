set(MKD_PLATFORM_MACOS_OBJCXX_SOURCES
        folder_prompt.mm
)

set(MKD_PLATFORM_MACOS_SOURCES
        bookmark_store.cpp
        fs.cpp
        macos_dictionary_source.cpp
        monokakido_receipt.cpp
        parallel.cpp
        scoped_security_access.cpp
)

target_sources(mkd PRIVATE
        ${MKD_PLATFORM_MACOS_SOURCES}
        ${MKD_PLATFORM_MACOS_OBJCXX_SOURCES}
)

target_link_libraries(mkd PRIVATE
        CRYPTOKIT_CPP
        "-framework Foundation"
        "-framework Security"
        "-framework CoreServices"
        "-framework Cocoa"
)

set_source_files_properties(${MKD_PLATFORM_MACOS_OBJCXX_SOURCES} PROPERTIES COMPILE_FLAGS "-fno-objc-arc")
