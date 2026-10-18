//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "reading.hpp"
#include <MKD/dictionary/dictionary_product.hpp>
#include <nanobind/stl/filesystem.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/string_view.h>

namespace MKD::Python
{
    nb::typed<nb::tuple, Dictionary, nb::ellipsis> productDictionaries(nb::handle self)
    {
        return borrowedTuple<Dictionary>(self, nb::cast<const DictionaryProduct&>(self).dictionaries());
    }

    nb::typed<nb::tuple, AppendixEntryList, nb::ellipsis> dictionaryAppendices(nb::handle self)
    {
        return borrowedTuple<AppendixEntryList>(self, nb::cast<const Dictionary&>(self).appendixEntryLists());
    }

    nb::str entryXml(const Dictionary& dictionary, const uint32_t pageId)
    {
        RetainedSpan data;
        std::string_view text;
        {
            nb::gil_scoped_release release;
            data = unwrap(dictionary.entryById(pageId)).data;
            text = unwrap(data.utf8());
        }
        return nb::str(text.data(), text.size());
    }

    void bindProduct(nb::module_& module)
    {
        auto type = nb::class_<DictionaryProduct>(module, "DictionaryProduct",
                                                  "Open product metadata and dictionaries. Child dictionaries retain this product.");
        type.def_static(
            "open", [](const std::filesystem::path& path) { return unwrap(DictionaryProduct::openAtPath(path)); },
            "path"_a, nb::call_guard<nb::gil_scoped_release>());

        type.def_static(
            "load_metadata",
            [](const std::filesystem::path& path) { return unwrap(DictionaryProduct::loadMetadata(path)); }, "path"_a,
            nb::call_guard<nb::gil_scoped_release>(), "Read metadata without opening resource stores.");

        type.def_prop_ro("metadata", [](const DictionaryProduct& product) { return product.metadata(); });
        type.def_prop_ro("display_title", &DictionaryProduct::displayTitle);
        type.def_prop_ro("path", [](const DictionaryProduct& product) { return product.paths().productRoot(); });
        type.def_prop_ro("icon_path", &DictionaryProduct::iconPath);
        type.def_prop_ro("dictionaries", &productDictionaries);

        type.def("find_content", nb::overload_cast<std::string_view>(&DictionaryProduct::findContent),
                 "identifier"_a, nb::rv_policy::reference_internal);
    }

    void bindDictionaryResources(nb::class_<Dictionary>& type)
    {
        type.def_prop_ro("entries", nb::overload_cast<>(&Dictionary::entries, nb::const_),
                         nb::rv_policy::reference_internal,
                         "Page store, or None when absent. Iteration reads one page at a time.");
        type.def_prop_ro("graphics", nb::overload_cast<>(&Dictionary::graphics, nb::const_),
                         nb::rv_policy::reference_internal);
        type.def_prop_ro("audio", [](nb::handle self)
                         -> nb::typed<nb::object, std::variant<std::monostate, ResourceStore, NamedResourceStore>>
                         {
                             const auto* audio = nb::cast<const Dictionary&>(self).audio();
                             if (!audio)
                                 return nb::none();

                             return std::visit([&](const auto& store)
                             {
                                 return nb::cast(&store, nb::rv_policy::reference_internal, self);
                             }, *audio);
                         });
        type.def_prop_ro("fonts", [](nb::handle self)
        {
            return borrowedTuple<Font>(self, nb::cast<const Dictionary&>(self).fonts());
        });
        type.def_prop_ro("headlines", [](nb::handle self)
        {
            return borrowedTuple<HeadlineStore>(self, nb::cast<const Dictionary&>(self).headlines());
        });
        type.def_prop_ro("keystores", [](nb::handle self)
        {
            return borrowedTuple<Keystore>(self, nb::cast<const Dictionary&>(self).keystores());
        });
        type.def_prop_ro("stylesheet", nb::overload_cast<>(&Dictionary::stylesheet, nb::const_),
                         nb::rv_policy::reference_internal);
        type.def_prop_ro("nightmode_stylesheet", nb::overload_cast<>(&Dictionary::nightmodeStylesheet, nb::const_),
                         nb::rv_policy::reference_internal);
    }

    void bindDictionary(nb::module_& module)
    {
        auto type = nb::class_<Dictionary>(module, "Dictionary",
                                           "Read a content dictionary. Stores decode payloads on demand and retain their owners.");
        type.def_prop_ro("id", &Dictionary::id);
        type.def_prop_ro("content", [](const Dictionary& dictionary) { return dictionary.content(); });
        type.def_prop_ro("presentation",
                         [](const Dictionary& dictionary) { return dictionary.presentation(); });
        type.def_prop_ro("appendix_entry_lists", &dictionaryAppendices);
        type.def_prop_ro("entry_order", &Dictionary::entryOrder, nb::rv_policy::reference_internal);
        bindDictionaryResources(type);

        type.def_prop_ro("entry_count", [](const Dictionary& dictionary)
        {
            return dictionary.resourceCount(ResourceType::Contents);
        });

        type.def(
            "entry_data", [](const Dictionary& dictionary, uint32_t pageId)
            {
                return unwrap(dictionary.entryById(pageId)).data;
            }, "page_id"_a,
            nb::call_guard<nb::gil_scoped_release>());

        type.def("entry_xml", &entryXml, "page_id"_a, "Read a page as an owning Unicode string.");

        type.def(
            "resource", [](const Dictionary& dictionary, std::string_view path)
            {
                return unwrap(dictionary.resource(path));
            }, "path"_a,
            nb::call_guard<nb::gil_scoped_release>(),
            "Read a known content-relative loose or supported packed resource as a retained Buffer.");

        type.def(
            "headline_for_entry_id", [](const Dictionary& dictionary, EntryId id)
            {
                return unwrap(dictionary.headlineForEntryId(id));
            }, "entry_id"_a,
            nb::call_guard<nb::gil_scoped_release>());

        type.def(
            "sorting_headline_for_entry_id", [](const Dictionary& dictionary, EntryId id)
            {
                return unwrap(dictionary.sortingHeadlineForEntryId(id));
            }, "entry_id"_a,
            nb::call_guard<nb::gil_scoped_release>());
    }
}
