target_sources(mkd PRIVATE
        dictionary.cpp
        dictionary_product.cpp
        dictionary_search.cpp
        metadata.cpp
        content_info.cpp
        paths.cpp
        text_normalize.cpp
        product_writer.cpp
        dictionary_writer.cpp
        creation_metadata.cpp
        creation_page.cpp
        creation_session.cpp
)

target_link_libraries(mkd PUBLIC glaze::glaze)

add_subdirectory(search)
