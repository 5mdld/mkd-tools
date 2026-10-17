//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "bindings.hpp"
#include "appendix.hpp"
#include "unicode/utf8.hpp"
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>

namespace MKD::Python
{
    AppendixEntry AppendixItem::asNative() const
    {
        std::optional<char16_t> first;
        if (firstCharacter)
        {
            const auto text = detail::unicode::toUtf16(*firstCharacter);
            if (text.size() != 1)
                throw nb::value_error("First character must be one UTF-16 code unit");

            first = text.front();
        }
        return {entryId, label ? std::optional<std::string_view>(*label) : std::nullopt, first, dictionaryId};
    }

    AppendixItem AppendixItem::copy(const AppendixEntry& entry)
    {
        return {
            entry.entryId,
            entry.label ? std::optional<std::string>(*entry.label) : std::nullopt,
            entry.firstCharacter
                ? std::optional(detail::unicode::toUtf8(std::u16string_view(&*entry.firstCharacter, 1)))
                : std::nullopt,
            entry.dictionaryId,
        };
    }

    void bindAppendixEntry(nb::module_& module)
    {
        auto type = nb::class_<AppendixItem>(module, "AppendixEntry");
        type.def(nb::init<EntryId, std::optional<std::string>, std::optional<std::string>>(), "entry_id"_a,
                 "label"_a = nb::none(), "first_character"_a = nb::none());

        type.def_ro("entry_id", &AppendixItem::entryId);
        type.def_ro("dictionary_id", &AppendixItem::dictionaryId);
        type.def_ro("label", &AppendixItem::label);
        type.def_ro("first_character", &AppendixItem::firstCharacter);
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
            "entry_at", [](const AppendixEntryList& list, size_t index)
            {
                return AppendixItem::copy(unwrap(list.entryAt(index)));
            }, "index"_a,
            nb::call_guard<nb::gil_scoped_release>());
    }
}
