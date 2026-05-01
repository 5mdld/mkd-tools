target_sources(mkd PRIVATE
        dictionary.cpp
        dictionary_product.cpp
        dictionary_search.cpp
        metadata.cpp
        paths.cpp
        text_normalize.cpp
)

target_link_libraries(mkd PUBLIC glaze::glaze)

add_subdirectory(search)
