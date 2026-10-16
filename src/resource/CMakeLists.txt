target_sources(mkd PRIVATE
        appendix_entry_list.cpp
        retained_span.cpp
        font.cpp
        resource_loader.cpp
        stylesheet.cpp
)

add_subdirectory(detail)
add_subdirectory(headlines)
add_subdirectory(keystore)
add_subdirectory(nrsc)
add_subdirectory(rsc)
