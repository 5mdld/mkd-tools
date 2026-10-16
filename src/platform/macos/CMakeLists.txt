add_subdirectory(bindings)

target_sources(mkd PRIVATE
        paths.cpp
        chacha_poly.cpp
        parallel.cpp
)

target_link_libraries(mkd PRIVATE
        CRYPTOKIT_CPP
        "-framework Foundation"
        "-framework Security"
        "-framework CoreServices"
)
