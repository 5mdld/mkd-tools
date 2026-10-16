target_sources(mkd PRIVATE
        sequential_blob_writer.cpp
        zlib_stream.cpp
)

target_link_libraries(mkd PUBLIC ZLIB::ZLIB)
