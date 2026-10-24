set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/authoring/resources/product_information.css")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/authoring/resources/product_information.css" MKD_PRODUCT_INFORMATION_CSS)
configure_file(authoring/resources/product_information_style.hpp.in product_information_style.hpp @ONLY)
target_include_directories(mkd PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

target_sources(mkd PRIVATE
        navigation/navigation.cpp
        navigation/native_navigation.cpp
        authoring/navigation_writer.cpp
        navigation/appendix_reader.cpp
        authoring/appendix_writer.cpp
        content/inline_text.cpp
        content/native_content.cpp
        package/resource_resolver.cpp
        content_references.cpp
        package/resource_loader.cpp
        entry_order.cpp
        dictionary.cpp
        dictionary_product.cpp
        dictionary_search.cpp
        package/metadata.cpp
        package/content_info.cpp
        package/paths.cpp
        search/text_normalize.cpp
        authoring/product_writer.cpp
        authoring/product_information.cpp
        authoring/product_icon.cpp
        authoring/dictionary_writer.cpp
        authoring/entry_writer.cpp
        creation_metadata.cpp
        creation_page.cpp
        creation_session.cpp
        creation_search.cpp
)

target_link_libraries(mkd PUBLIC glaze::glaze)

add_subdirectory(search)
