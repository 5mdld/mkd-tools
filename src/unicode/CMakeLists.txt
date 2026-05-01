target_sources(mkd PRIVATE
        normalization.cpp
        search_folding.cpp
        unicode.cpp
        utf8.cpp
)

target_link_libraries(mkd PRIVATE utf8proc::utf8proc)
