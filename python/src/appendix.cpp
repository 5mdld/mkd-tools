//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "bindings.hpp"
#include <MKD/resource/appendix_entry_list.hpp>
#include "unicode/utf8.hpp"
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>

namespace MKD::Python
{
    void bindAppendixEntry(nb::module_& module)
    {
        auto type = nb::class_<AppendixEntry>(module, "AppendixEntry");
        type.def("__init__", [](AppendixEntry* self, const EntryId id, std::optional<std::string> label,
                                std::optional<std::string> firstCharacter)
        {
            std::optional<char16_t> first;
            if (firstCharacter)
            {
                const auto text = detail::unicode::toUtf16(*firstCharacter);
                if (text.size() != 1)
                    throw nb::value_error("First character must be one UTF-16 code unit");

                first = text.front();
            }
            new(self) AppendixEntry{id, std::move(label), first};
        }, "entry_id"_a, "label"_a = nb::none(), "first_character"_a = nb::none());

        type.def_ro("entry_id", &AppendixEntry::entryId);
        type.def_ro("dictionary_id", &AppendixEntry::dictionaryId);
        type.def_ro("label", &AppendixEntry::label);
        type.def_prop_ro("first_character", [](const AppendixEntry& entry) -> std::optional<std::string>
        {
            if (!entry.firstCharacter) return std::nullopt;
            return detail::unicode::toUtf8(std::u16string_view(&*entry.firstCharacter, 1));
        });
    }

    void bindAppendixList(nb::module_& module)
    {
        auto type = nb::class_<AppendixEntryList>(module, "AppendixEntryList");
        type.def("__len__", &AppendixEntryList::size);

        type.def_prop_ro("rows", &AppendixEntryList::rows);
        type.def_prop_ro("columns", &AppendixEntryList::columns);
        type.def_prop_ro("filename", &AppendixEntryList::filename);
        type.def_prop_ro("title", &AppendixEntryList::title);

        type.def(
            "entry_at", [](const AppendixEntryList& list, const size_t index)
            {
                return unwrap(list.entryAt(index)).copy();
            }, "index"_a,
            nb::call_guard<nb::gil_scoped_release>());
    }
}
