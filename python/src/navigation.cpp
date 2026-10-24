//
// kiwakiwaaにより 2026/10/19 に作成されました。
//

#include "reading.hpp"
#include <MKD/dictionary/navigation.hpp>
#include <MKD/dictionary/dictionary_product.hpp>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/optional.h>

namespace MKD::Python
{
    void bindNavigationLabels(nb::module_& module)
    {
        auto radical = nb::class_<RadicalLabel>(module, "RadicalLabel");
        radical.def(nb::init<std::string, std::string, std::string>(), "character"_a,
                    nb::kw_only(), "variants"_a = "", "reading"_a = "");
        radical.def_ro("character", &RadicalLabel::character);
        radical.def_ro("variants", &RadicalLabel::variants);
        radical.def_ro("reading", &RadicalLabel::reading);
        auto reference = nb::class_<NavigationReference>(module, "NavigationReference");
        reference.def(nb::init<std::string, std::vector<uint32_t>>(), "path"_a,
                      nb::kw_only(), "section_path"_a = std::vector<uint32_t>{});
        reference.def_ro("path", &NavigationReference::path);
        reference.def_ro("section_path", &NavigationReference::sectionPath);
    }

    void bindNavigationIndex(nb::module_& module)
    {
        nb::enum_<NavigationKind>(module, "NavigationKind")
            .value("GENERIC", NavigationKind::Generic).value("ROOT", NavigationKind::Root)
            .value("RADICALS", NavigationKind::Radicals).value("READINGS", NavigationKind::Readings)
            .value("TOTAL_STROKES", NavigationKind::TotalStrokes).value(
                "CLASSIFICATIONS", NavigationKind::Classifications)
            .value("NAMES", NavigationKind::Names).value("IMAGES", NavigationKind::Images).value(
                "ETYMOLOGY", NavigationKind::Etymology);
        auto index = nb::class_<NavigationIndex>(module, "NavigationIndex");
        auto section = nb::class_<NavigationSection>(module, "NavigationSection");
        auto item = nb::class_<NavigationItem>(module, "NavigationItem");
        index.def(
            nb::init<NavigationKind, std::vector<NavigationSection>, bool, bool, bool, uint32_t, uint32_t, bool,
                     bool>(),
            nb::kw_only(), "kind"_a = NavigationKind::Generic, "sections"_a = std::vector<NavigationSection>{},
            "compact"_a = false, "section_index"_a = true, "parent_headlines"_a = false,
            "rows"_a = 0, "columns"_a = 0, "vertical"_a = true, "shorten_headlines"_a = false);
        index.def_ro("kind", &NavigationIndex::kind);
        index.def_ro("sections", &NavigationIndex::sections);
        index.def_ro("compact", &NavigationIndex::compact);
        index.def_ro("section_index", &NavigationIndex::sectionIndex);
        index.def_ro("parent_headlines", &NavigationIndex::parentHeadlines);
        index.def_ro("rows", &NavigationIndex::rows);
        index.def_ro("columns", &NavigationIndex::columns);
        index.def_ro("vertical", &NavigationIndex::vertical);
        index.def_ro("shorten_headlines", &NavigationIndex::shortenHeadlines);
        section.def(nb::init<std::string, std::vector<NavigationItem>>(), "title"_a = "",
                    "items"_a = std::vector<NavigationItem>{});
        section.def_ro("title", &NavigationSection::title);
        section.def_ro("items", &NavigationSection::items);
        item.def(nb::new_([](NavigationTarget target, NavigationLabel label)
        {
            return NavigationItem{std::move(label), std::move(target)};
        }), nb::kw_only(), "target"_a = std::monostate{}, "label"_a = std::string{});
        item.def(nb::new_([](const EntryId target, NavigationLabel label)
        {
            return NavigationItem{std::move(label), AppendixEntryLink{target, 0}};
        }), nb::kw_only(), "target"_a, "label"_a = std::string{});
        item.def_ro("label", &NavigationItem::label);
        item.def_ro("target", &NavigationItem::target);
    }

    void bindNavigationView(nb::module_& module)
    {
        auto view = nb::class_<NavigationView>(module, "NavigationView");
        auto item = nb::class_<NavigationItemView>(module, "NavigationItemView");
        view.def_prop_ro("kind_code", &NavigationView::kindCode);
        view.def_prop_ro("section_count", &NavigationView::sectionCount);
        view.def("section_title", [](const NavigationView& view, const size_t section)
        {
            return unwrap(view.sectionTitle(section));
        }, "section"_a);
        view.def("item_count", [](const NavigationView& view, const size_t section)
        {
            return unwrap(view.itemCount(section));
        }, "section"_a);
        view.def("item_at", [](const NavigationView& view, const size_t section, const size_t index)
        {
            return unwrap(view.itemAt(section, index));
        }, "section"_a, "index"_a, nb::call_guard<nb::gil_scoped_release>());
        item.def_ro("label", &NavigationItemView::label);
        item.def_ro("target", &NavigationItemView::target);
    }
}
