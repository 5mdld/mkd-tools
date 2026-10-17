//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "bindings.hpp"
#include <MKD/platform/directory_dictionary_source.hpp>
#ifdef __APPLE__
#include <MKD/platform/macos/paths.hpp>
#endif
#include <nanobind/stl/filesystem.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/vector.h>

namespace MKD::Python
{
    void bindDiscovery(nb::module_& module)
    {
        nb::class_<DictionaryInfo>(module, "DictionaryInfo")
            .def_ro("id", &DictionaryInfo::id)
            .def_ro("path", &DictionaryInfo::path);
        auto type = nb::class_<DirectoryDictionarySource>(module, "DirectoryDictionarySource");
        type.def(nb::init<std::filesystem::path>(), "root"_a);
        type.def("find_all_available", [](const DirectoryDictionarySource& source)
                 {
                     return unwrap(source.findAllAvailable());
                 }, nb::call_guard<nb::gil_scoped_release>(),
                 "Discover products without opening their content stores.");
        type.def("find_by_id", [](const DirectoryDictionarySource& source, std::string_view id)
        {
            return unwrap(source.findById(id));
        }, "id"_a, nb::call_guard<nb::gil_scoped_release>());
#ifdef __APPLE__
        module.def("monokakido_data_directory", [] { return unwrap(macOS::monokakidoDataDirectory()); });
        module.def("dictionaries_directory", &macOS::dictionariesDirectory, "data_directory"_a);
#endif
    }
}
