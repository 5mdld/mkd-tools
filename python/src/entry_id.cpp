//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "bindings.hpp"
#include <MKD/resource/entry_id.hpp>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <format>

namespace MKD::Python
{
    void bindEntryId(nb::module_& module)
    {
        auto type = nb::class_<EntryId>(module, "EntryId");
        type.def(nb::init<uint32_t, uint16_t>(), "page_id"_a, "item_id"_a = 0);

        type.def_ro("page_id", &EntryId::pageId).def_ro("item_id", &EntryId::itemId);

        type.def(nb::self == nb::self);

        type.def("__str__", &EntryId::toString);

        type.def("__hash__", [](EntryId id) { return std::hash<EntryId>{}(id); });

        type.def("__repr__", [](EntryId id)
        {
            return std::format("EntryId(page_id={}, item_id={})", id.pageId, id.itemId);
        });
    }
}
