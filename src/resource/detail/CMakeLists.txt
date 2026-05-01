target_sources(mkd PRIVATE
        binary_buffer.cpp
        sequential_blob_writer.cpp
        zlib_stream.cpp
)

target_link_libraries(mkd PUBLIC ZLIB::ZLIB)
