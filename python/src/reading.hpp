//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "bindings.hpp"

namespace MKD::Python
{
    inline size_t sequenceIndex(Py_ssize_t index, size_t size)
    {
        if (index < 0)
            index += static_cast<Py_ssize_t>(size);

        if (index < 0 || static_cast<size_t>(index) >= size)
            throw nb::index_error();

        return static_cast<size_t>(index);
    }

    template <typename T>
    nb::typed<nb::iterator, T> sequenceIterator(nb::handle self)
    {
        auto* iterator = PySeqIter_New(self.ptr());
        if (!iterator)
            nb::raise_python_error();

        return nb::steal<nb::iterator>(iterator);
    }

    template <typename T, typename Range>
    nb::typed<nb::tuple, T, nb::ellipsis> borrowedTuple(nb::handle owner, const Range& range)
    {
        nb::list values;
        for (const auto& value : range)
            values.append(nb::cast(&value, nb::rv_policy::reference_internal, owner));

        return nb::tuple(values);
    }
}
