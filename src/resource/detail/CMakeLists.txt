target_sources(mkd PRIVATE
        resource_files.cpp
        sequential_blob_writer.cpp
        zlib.cpp
)

target_link_libraries(mkd PUBLIC ZLIB::ZLIB)
