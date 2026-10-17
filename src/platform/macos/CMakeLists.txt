add_subdirectory(bindings)

target_sources(mkd PRIVATE
        paths.cpp
        search_lemma.cpp
        chacha_poly.cpp
        parallel.cpp
)

target_link_libraries(mkd PRIVATE
        mkd-macos-bindings
        "-framework Foundation"
        "-framework Security"
        "-framework CoreServices"
)
