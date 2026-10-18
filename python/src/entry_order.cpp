//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "reading.hpp"
#include <MKD/dictionary/entry_order.hpp>
#include <nanobind/stl/optional.h>

namespace MKD::Python
{
    void bindEntryOrder(nb::module_& module)
    {
        auto type = nb::class_<EntryOrder>(module, "EntryOrder", "Read-only article browsing order. No XML is loaded.");
        type.def("__len__", &EntryOrder::size);
        type.def("__iter__", &sequenceIterator<uint32_t>);
        type.def("__getitem__", [](const EntryOrder& order, Py_ssize_t index)
        {
            return unwrap(order.pageAt(sequenceIndex(index, order.size())));
        }, "index"_a);
        type.def("index", [](const EntryOrder& order, uint32_t pageId) { return unwrap(order.indexOf(pageId)); },
                 "page_id"_a);
        type.def("previous", [](const EntryOrder& order, uint32_t pageId) { return unwrap(order.previous(pageId)); },
                 "page_id"_a);
        type.def("next", [](const EntryOrder& order, uint32_t pageId) { return unwrap(order.next(pageId)); },
                 "page_id"_a);
    }
}
