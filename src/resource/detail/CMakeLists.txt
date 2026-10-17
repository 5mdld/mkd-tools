target_sources(mkd PRIVATE
        resource_files.cpp
        segmented_file_writer.cpp
        fragment_writer.cpp
        zlib.cpp
)

target_link_libraries(mkd PUBLIC ZLIB::ZLIB)
