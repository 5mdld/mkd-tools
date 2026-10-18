execute_process(
        COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
        OUTPUT_VARIABLE nanobind_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY
)
find_package(nanobind CONFIG REQUIRED)

set_target_properties(mkd utf8proc pugixml PROPERTIES POSITION_INDEPENDENT_CODE ON)
if (TARGET zlibstatic)
    set_target_properties(zlibstatic PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif ()

nanobind_add_module(_mkd
        src/module.cpp
        src/entry_id.cpp
        src/metadata.cpp
        src/inline_text.cpp
        src/buffer.cpp
        src/appendix.cpp
        src/dictionary.cpp
        src/entry_order.cpp
        src/resources.cpp
        src/indexes.cpp
        src/discovery.cpp
        src/export.cpp
        src/search.cpp
        src/writing.cpp
        src/entry_writer.cpp
)
target_link_libraries(_mkd PRIVATE MKD::mkd)
target_include_directories(_mkd PRIVATE "${PROJECT_SOURCE_DIR}/src")
install(TARGETS _mkd LIBRARY DESTINATION mkd RUNTIME DESTINATION mkd)

nanobind_add_stub(mkd-python-stubs
        MODULE _mkd
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/_mkd.pyi"
        PYTHON_PATH "$<TARGET_FILE_DIR:_mkd>"
        DEPENDS _mkd
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/_mkd.pyi" DESTINATION mkd)
