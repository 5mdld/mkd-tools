//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "bindings.hpp"
#include <MKD/dictionary/entry_writer.hpp>
#include <nanobind/stl/string.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string_view.h>

namespace MKD::Python
{
    void bindEntryWriter(nb::module_& module)
    {
        nb::enum_<EntryTargetKind>(module, "EntryTargetKind")
            .value("IDIOM", EntryTargetKind::Idiom)
            .value("COMPOUND", EntryTargetKind::Compound)
            .value("EXAMPLE", EntryTargetKind::Example)
            .value("SENSE", EntryTargetKind::Sense)
            .value("SECTION", EntryTargetKind::Section);

        auto type = nb::class_<EntryWriter>(module, "EntryWriter",
                                            "Reserve targets, then write a complete XML document once.");
        type.def_prop_ro("target", &EntryWriter::target);
        type.def("add_target", [](const EntryWriter& entry, EntryTargetKind kind, std::optional<SourceContext> source)
        {
            return unwrap(entry.addTarget(kind, std::move(source)));
        }, "kind"_a, nb::kw_only(), "source"_a = nb::none());
        type.def("write_xml", [](const EntryWriter& entry, std::string_view xml, std::optional<SourceContext> source)
        {
            unwrap(entry.writeXml(xml, std::move(source)));
        }, "xml"_a, nb::kw_only(), "source"_a = nb::none());
    }
}
