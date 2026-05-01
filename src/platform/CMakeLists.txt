target_sources(mkd PRIVATE
        directory_dictionary_source.cpp
        mmap_file.cpp
)

if (NOT APPLE)
    target_sources(mkd PRIVATE
            parallel.cpp
    )
endif ()

if (APPLE)
    add_subdirectory(macos)
endif ()
