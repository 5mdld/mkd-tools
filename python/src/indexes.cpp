//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "reading.hpp"
#include <MKD/dictionary/dictionary_search.hpp>
#include <MKD/resource/headline_store.hpp>
#include <MKD/resource/keystore.hpp>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/vector.h>

namespace MKD::Python
{
    void bindHeadlineStore(nb::module_& module)
    {
        auto type = nb::class_<HeadlineStore>(module, "HeadlineStore",
                                              "Headline components decoded one record at a time.");
        type.def_prop_ro("filename", &HeadlineStore::filename);
        type.def("__len__", &HeadlineStore::size);
        type.def("__iter__", &sequenceIterator<HeadlineComponents>);
        type.def("__getitem__", [](const HeadlineStore& store, Py_ssize_t index)
                 {
                     return unwrap(store.entryAt(sequenceIndex(index, store.size())));
                 }, "index"_a,
                 nb::call_guard<nb::gil_scoped_release>());
        type.def("entry_id_at", [](const HeadlineStore& store, Py_ssize_t index)
                 {
                     return unwrap(store.entryIdAt(sequenceIndex(index, store.size())));
                 }, "index"_a,
                 nb::call_guard<nb::gil_scoped_release>());
        type.def("components_for_entry_id", [](const HeadlineStore& store, EntryId id, bool fallback)
                 {
                     return unwrap(store.componentsForEntryId(id, fallback));
                 }, "entry_id"_a,
                 nb::kw_only(), "allow_fallback"_a = true, nb::call_guard<nb::gil_scoped_release>());
    }

    void bindKeystore(nb::module_& module)
    {
        nb::enum_<KeystoreIndex>(module, "KeystoreIndex")
            .value("LENGTH", KeystoreIndex::Length)
            .value("PREFIX", KeystoreIndex::Prefix)
            .value("SUFFIX", KeystoreIndex::Suffix)
            .value("OTHER", KeystoreIndex::Other);
        nb::enum_<KeystoreIdMapping>(module, "KeystoreIdMapping")
            .value("STORED", KeystoreIdMapping::Stored)
            .value("RESOLVED", KeystoreIdMapping::Resolved);
        auto type = nb::class_<Keystore>(module, "Keystore",
                                         "Inspect stored keys and targets. Use DictionarySearch for dictionary search rules.");
        type.def_prop_ro("filename", &Keystore::filename);
        type.def_prop_ro("scope", [](const Keystore& store) -> std::optional<SearchScope>
        {
            return store.scope().transform([](KeystoreScope scope)
            {
                return static_cast<SearchScope>(scope);
            });
        });
        type.def("index_size", &Keystore::indexSize, "index_type"_a);
        type.def("key_at", [](const Keystore& store, KeystoreIndex type, size_t index)
                 {
                     return unwrap(store.keyAt(type, index));
                 }, "index_type"_a, "index"_a,
                 nb::call_guard<nb::gil_scoped_release>());
        type.def("entry_ids_at", [](const Keystore& store, KeystoreIndex type, size_t index, KeystoreIdMapping mapping)
                 {
                     return unwrap(store.entryIdsAt(type, index, mapping));
                 }, "index_type"_a, "index"_a,
                 nb::kw_only(), "mapping"_a = KeystoreIdMapping::Resolved, nb::call_guard<nb::gil_scoped_release>());
        type.def("keys_for_entry", [](const Keystore& store, EntryId id)
                 {
                     return unwrap(store.keysForEntry(id));
                 }, "entry_id"_a, nb::call_guard<nb::gil_scoped_release>(),
                 "Build the inverse index on first use, then return this entry's keys.");
    }
}
