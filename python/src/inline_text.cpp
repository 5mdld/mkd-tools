//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "bindings.hpp"
#include <MKD/dictionary/inline_text.hpp>
#include <MKD/resource/headline_store.hpp>
#include "unicode/utf8.hpp"
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>

namespace MKD::Python
{
    void bindHeadline(nb::module_& module)
    {
        auto type = nb::class_<HeadlineComponents>(module, "HeadlineComponents");
        type.def(
            "__init__",
            [](HeadlineComponents* self, std::string headline, EntryId entryId, std::string sorting,
               std::string prefix, std::string suffix)
            {
                new(self) HeadlineComponents{
                    .prefix = detail::unicode::toUtf16(prefix),
                    .headline = detail::unicode::toUtf16(headline),
                    .suffix = detail::unicode::toUtf16(suffix),
                    .sortingHeadline = detail::unicode::toUtf16(sorting),
                    .entryId = entryId,
                };
            },
            "headline"_a, "entry_id"_a, nb::kw_only(), "sorting_headline"_a = "", "prefix"_a = "",
            "suffix"_a = "");

        type.def_rw("entry_id", &HeadlineComponents::entryId);

        type.def_prop_rw(
            "headline",
            [](const HeadlineComponents& value) { return detail::unicode::toUtf8(value.headline); },
            [](HeadlineComponents& value, const std::string& text)
            {
                value.headline = detail::unicode::toUtf16(text);
            });

        type.def_prop_rw(
            "sorting_headline",
            [](const HeadlineComponents& value) { return detail::unicode::toUtf8(value.sortingHeadline); },
            [](HeadlineComponents& value, const std::string& text)
            {
                value.sortingHeadline = detail::unicode::toUtf16(text);
            });

        type.def_prop_rw(
            "prefix", [](const HeadlineComponents& value) { return detail::unicode::toUtf8(value.prefix); },
            [](HeadlineComponents& value, const std::string& text)
            {
                value.prefix = detail::unicode::toUtf16(text);
            });

        type.def_prop_rw(
            "suffix", [](const HeadlineComponents& value) { return detail::unicode::toUtf8(value.suffix); },
            [](HeadlineComponents& value, const std::string& text)
            {
                value.suffix = detail::unicode::toUtf16(text);
            });

        type.def("full",
                 [](const HeadlineComponents& value) { return detail::unicode::toUtf8(value.full()); });
    }

    void bindInlineElements(nb::module_& module)
    {
        nb::class_<GaijiResource>(module, "GaijiResource")
            .def(nb::init<std::string>(), "path"_a)
            .def_ro("path", &GaijiResource::path);
        nb::enum_<GaijiRendering>(module, "GaijiRendering")
            .value("TEXT", GaijiRendering::Text)
            .value("ORIGINAL", GaijiRendering::Original);

        nb::class_<Ruby>(module, "Ruby")
            .def(nb::init<std::string, std::string>(), "base"_a, "reading"_a)
            .def_ro("base", &Ruby::base)
            .def_ro("reading", &Ruby::reading);
        nb::class_<Gaiji>(module, "Gaiji")
            .def(nb::init<GaijiResource, std::string, GaijiRendering>(), "resource"_a, "replacement"_a,
                 "rendering"_a = GaijiRendering::Text)
            .def_ro("resource", &Gaiji::resource)
            .def_ro("replacement", &Gaiji::replacement)
            .def_ro("rendering", &Gaiji::rendering);
        nb::class_<EntryLink>(module, "EntryLink")
            .def(nb::init<EntryId, std::string>(), "target"_a, "text"_a)
            .def_ro("target", &EntryLink::target)
            .def_ro("text", &EntryLink::text);
        nb::class_<HeadlineMarkup>(module, "HeadlineMarkup")
            .def(nb::init<std::string>(), "text"_a)
            .def_ro("text", &HeadlineMarkup::text);
    }

    void bindInlineText(nb::module_& module)
    {
        auto type = nb::class_<InlineText>(module, "InlineText");
        type.def(nb::init<>());

        type.def("append", &InlineText::append, "element"_a);

        type.def_prop_ro("elements",
                         [](const InlineText& text)
                         {
                             nb::list elements;
                             for (const auto& element : text.elements())
                                 elements.append(nb::cast(element));
                             return nb::tuple(elements);
                         });

        type.def_static(
            "from_headline", [](const std::string& text)
            {
                return unwrap(InlineText::fromHeadline(detail::unicode::toUtf16(text)));
            }, "text"_a);

        type.def("to_headline",
                 [](const InlineText& text) { return detail::unicode::toUtf8(unwrap(text.toHeadline())); });

        type.def("to_xml", [](const InlineText& text) { return unwrap(text.toXhtml()); });

        type.def("plain_text", [](const InlineText& text) { return unwrap(text.plainText()); });
    }
}
