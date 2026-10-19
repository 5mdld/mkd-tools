//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "reading.hpp"
#include <MKD/dictionary/appendix_menu.hpp>
#include <MKD/dictionary/dictionary_product.hpp>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

namespace MKD::Python
{
    void bindProductAppendices(nb::class_<DictionaryProduct>& type)
    {
        type.def_prop_ro("navigation", [](const DictionaryProduct& product)
        {
            return unwrap(product.navigation());
        }, nb::call_guard<nb::gil_scoped_release>());
        type.def_prop_ro("appendices", &DictionaryProduct::appendices, nb::rv_policy::reference_internal);
        type.def(
            "resource",
            [](const DictionaryProduct& product, std::string_view path)
            {
                return unwrap(product.resource(path));
            },
            "path"_a, nb::call_guard<nb::gil_scoped_release>());
        type.def(
            "open_appendix_list",
            [](const DictionaryProduct& product, const AppendixList& list)
            {
                return unwrap(product.openAppendixList(list));
            },
            "target"_a, nb::call_guard<nb::gil_scoped_release>());
        type.def(
            "open_appendix_document",
            [](const DictionaryProduct& product, const AppendixDocument& document)
            {
                return unwrap(product.openAppendixDocument(document));
            },
            "target"_a, nb::call_guard<nb::gil_scoped_release>());
    }

    void bindAppendixTargets(nb::module_& module)
    {
        auto list = nb::class_<AppendixList>(module, "AppendixList");
        list.def(nb::init<std::string, bool, bool, bool>(), "path"_a, nb::kw_only(), "single_section"_a = false,
                 "using_anchor_id"_a = false, "skip_the"_a = false);
        list.def_ro("path", &AppendixList::path);
        list.def_ro("single_section", &AppendixList::singleSection);
        list.def_ro("using_anchor_id", &AppendixList::usingAnchorId);
        list.def_ro("skip_the", &AppendixList::skipThe);

        auto document = nb::class_<AppendixDocument>(module, "AppendixDocument");
        document.def(nb::init<std::string, std::optional<std::string>>(), "path"_a, nb::kw_only(),
                     "fragment"_a = nb::none());
        document.def_ro("path", &AppendixDocument::path);
        document.def_ro("fragment", &AppendixDocument::fragment);

        auto gallery = nb::class_<AppendixGallery>(module, "AppendixGallery");
        gallery.def(nb::init<std::vector<std::string>>(), "images"_a);
        gallery.def_ro("images", &AppendixGallery::images);

        auto link = nb::class_<AppendixEntryLink>(module, "AppendixEntryLink");
        link.def(nb::init<EntryId, uint16_t>(), "entry_id"_a, "dictionary_id"_a);
        link.def_ro("entry_id", &AppendixEntryLink::entryId);
        link.def_ro("dictionary_id", &AppendixEntryLink::dictionaryId);
    }

    void bindAppendixCustom(nb::module_& module)
    {
        auto custom = nb::class_<AppendixCustom>(module, "AppendixCustom");
        custom.def_ro("presenter", &AppendixCustom::presenter);
        custom.def_ro("contents", &AppendixCustom::contents);
        custom.def_ro("file", &AppendixCustom::file);
        custom.def_ro("detail", &AppendixCustom::detail);
    }

    void bindAppendixMenu(nb::module_& module)
    {
        auto menu = nb::class_<AppendixMenu>(module, "AppendixMenu");
        auto section = nb::class_<AppendixSection>(module, "AppendixSection");
        auto item = nb::class_<AppendixItem>(module, "AppendixItem");

        menu.def(nb::init<std::vector<AppendixSection>>(), "sections"_a = std::vector<AppendixSection>{});
        menu.def_prop_ro("sections",
                         [](nb::handle self)
                         {
                             return borrowedTuple<AppendixSection>(self, nb::cast<const AppendixMenu&>(self).sections);
                         });
        section.def(nb::init<std::string, std::vector<AppendixItem>>(), "name"_a, "items"_a);
        section.def_ro("name", &AppendixSection::name);
        section.def_prop_ro("items",
                            [](nb::handle self)
                            {
                                return borrowedTuple<AppendixItem>(self, nb::cast<const AppendixSection&>(self).items);
                            });
        item.def(nb::init<std::string, AppendixTarget, std::optional<std::string>, std::optional<std::string>,
                          std::optional<std::string>, std::optional<std::string>, uint32_t>(),
                 "name"_a, "target"_a, nb::kw_only(), "title"_a = nb::none(), "number"_a = nb::none(),
                 "description"_a = nb::none(), "image"_a = nb::none(), "indentation"_a = 0);
        item.def_ro("name", &AppendixItem::name);
        item.def_prop_ro("target",
                         [](nb::handle self) -> nb::object
                         {
                             return std::visit(
                                 [&](const auto& target)
                                 {
                                     return nb::cast(&target, nb::rv_policy::reference_internal, self);
                                 },
                                 nb::cast<const AppendixItem&>(self).target);
                         });
        item.def_ro("title", &AppendixItem::title);
        item.def_ro("number", &AppendixItem::number);
        item.def_ro("description", &AppendixItem::description);
        item.def_ro("image", &AppendixItem::image);
        item.def_ro("indentation", &AppendixItem::indentation);
    }
}
