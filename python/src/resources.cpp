//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "reading.hpp"
#include <MKD/resource/font.hpp>
#include <MKD/resource/named_resource_store.hpp>
#include <MKD/resource/stylesheet.hpp>
#include <nanobind/stl/filesystem.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>

namespace MKD::Python
{
    template <typename Store, typename Item, typename Id>
    void bindResourceStore(nb::module_& module, const char* name)
    {
        auto type = nb::class_<Store>(module, name,
                                      "Read-only indexed store. Iteration reads one item at a time and retains its bytes.");
        type.def("__len__", &Store::size);
        type.def("__iter__", &sequenceIterator<Item>);
        type.def("__getitem__", [](const Store& store, Py_ssize_t index)
                 {
                     return unwrap(store.itemAt(sequenceIndex(index, store.size())));
                 }, "index"_a,
                 nb::call_guard<nb::gil_scoped_release>());
        type.def("id_at", [](const Store& store, Py_ssize_t index)
                 {
                     return unwrap(store.idAt(sequenceIndex(index, store.size())));
                 }, "index"_a,
                 nb::call_guard<nb::gil_scoped_release>(), "Read an ID without loading its payload.");
        type.def("get", [](const Store& store, Id id) { return unwrap(store.get(id)); }, "id"_a,
                 nb::call_guard<nb::gil_scoped_release>(), "Read by ID, returning a retained Buffer.");
    }

    void bindResourceStores(nb::module_& module)
    {
        nb::class_<ResourceStoreItem>(module, "ResourceStoreItem")
            .def_ro("id", &ResourceStoreItem::itemId)
            .def_ro("data", &ResourceStoreItem::data);
        nb::class_<NamedResourceStoreItem>(module, "NamedResourceStoreItem")
            .def_ro("id", &NamedResourceStoreItem::id)
            .def_ro("data", &NamedResourceStoreItem::data);
        bindResourceStore<ResourceStore, ResourceStoreItem, uint32_t>(module, "ResourceStore");
        bindResourceStore<NamedResourceStore, NamedResourceStoreItem, std::string_view>(module, "NamedResourceStore");
    }

    void bindFont(nb::module_& module)
    {
        auto type = nb::class_<Font>(module, "Font", "Mapped font or packed fragments, decoded on demand.");
        type.def_prop_ro("name", &Font::name);
        type.def_prop_ro("filename", &Font::filename);
        type.def("__len__", &Font::fragmentCount);
        type.def("__iter__", &sequenceIterator<RetainedSpan>);
        type.def("__getitem__", [](const Font& font, Py_ssize_t index)
                 {
                     return unwrap(font.fragmentAt(sequenceIndex(index, font.fragmentCount())));
                 }, "index"_a,
                 nb::call_guard<nb::gil_scoped_release>());
        type.def("write_to", [](const Font& font, const std::filesystem::path& path)
                 {
                     return unwrap(font.writeTo(path));
                 }, "path"_a, nb::call_guard<nb::gil_scoped_release>(),
                 "Stream to a new file and return bytes written. The parent directory must exist.");
    }

    void bindStylesheet(nb::module_& module)
    {
        auto type = nb::class_<Stylesheet>(module, "Stylesheet");
        type.def_prop_ro("name", &Stylesheet::name);
        type.def_prop_ro("data", &Stylesheet::data);
    }
}
