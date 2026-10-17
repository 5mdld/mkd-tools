target_sources(mkd PRIVATE
        directory_dictionary_source.cpp
        mmap_file.cpp
)

if (NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(mkd PRIVATE Threads::Threads)
    target_sources(mkd PRIVATE
            parallel.cpp
            search_lemma.cpp
    )
endif ()

if (APPLE)
    add_subdirectory(macos)
endif ()
