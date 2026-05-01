target_sources(mkd PRIVATE
        resource_store.cpp
        resource_store_contents.cpp
        resource_store_crypto.cpp
        resource_store_index.cpp
        resource_store_index_record.cpp
        resource_store_writer.cpp
)

if (APPLE)
    target_link_libraries(mkd PRIVATE "-framework Security")
elseif (WIN32)
    target_link_libraries(mkd PRIVATE bcrypt)
else ()
    find_package(OpenSSL REQUIRED)
    target_link_libraries(mkd PRIVATE OpenSSL::Crypto)
endif ()
