target_sources(mkd PRIVATE
        entry_id.cpp
        appendix_entry_list.cpp
        appendix_entry_list_writer.cpp
        retained_span.cpp
        font.cpp
        stylesheet.cpp
)

add_subdirectory(detail)
add_subdirectory(headlines)
add_subdirectory(keystore)
add_subdirectory(nrsc)
add_subdirectory(rsc)
