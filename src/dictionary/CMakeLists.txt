set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/product_information.css")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/resources/product_information.css" MKD_PRODUCT_INFORMATION_CSS)
configure_file(resources/product_information_style.hpp.in product_information_style.hpp @ONLY)
target_include_directories(mkd PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

target_sources(mkd PRIVATE
        inline_text.cpp
        native_content.cpp
        content_resources.cpp
        content_references.cpp
        dictionary.cpp
        dictionary_product.cpp
        dictionary_search.cpp
        metadata.cpp
        content_info.cpp
        paths.cpp
        text_normalize.cpp
        product_writer.cpp
        product_information.cpp
        dictionary_writer.cpp
        creation_metadata.cpp
        creation_page.cpp
        creation_session.cpp
)

target_link_libraries(mkd PUBLIC glaze::glaze)

add_subdirectory(search)
