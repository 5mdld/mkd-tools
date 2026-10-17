//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "bindings.hpp"
#include "buffer.hpp"
#include <MKD/resource/retained_span.hpp>
#include <nanobind/stl/filesystem.h>

namespace MKD::Python
{
    InputBytes::InputBytes(nb::handle object)
    {
        if (PyObject_GetBuffer(object.ptr(), &view_, PyBUF_CONTIG_RO) != 0)
            throw nb::python_error();

        if (view_.itemsize != 1 || view_.ndim != 1)
        {
            PyBuffer_Release(&view_);
            throw nb::type_error("Expected a contiguous, one-dimensional byte buffer");
        }
    }

    InputBytes::~InputBytes()
    {
        PyBuffer_Release(&view_);
    }

    std::span<const uint8_t> InputBytes::span() const
    {
        return {static_cast<const uint8_t*>(view_.buf), static_cast<size_t>(view_.len)};
    }

    struct BufferProtocol
    {
        static int get(PyObject* object, Py_buffer* view, int flags) noexcept
        {
            const auto* data = nb::inst_ptr<RetainedSpan>(nb::handle(object));
            if (data->size() > static_cast<size_t>(PY_SSIZE_T_MAX))
            {
                PyErr_SetString(PyExc_OverflowError, "Buffer is too large for Python");
                return -1;
            }
            // The exported buffer owns a reference to this retaining span.
            return PyBuffer_FillInfo(view, object, const_cast<uint8_t*>(data->data()),
                                     static_cast<Py_ssize_t>(data->size()), 1, flags);
        }

        inline static PyType_Slot slots[] = {{Py_bf_getbuffer, reinterpret_cast<void*>(get)}, {0, nullptr}};
    };

    void bindBuffer(nb::module_& module)
    {
        auto type = nb::class_<RetainedSpan>(module, "Buffer", nb::type_slots(BufferProtocol::slots));
        type.def_static(
            "map", [](const std::filesystem::path& path) { return unwrap(RetainedSpan::map(path)); }, "path"_a,
            nb::call_guard<nb::gil_scoped_release>());

        type.def("__len__", &RetainedSpan::size);

        type.def("__bytes__", [](const RetainedSpan& data) { return nb::bytes(data.data(), data.size()); });

        type.def("utf8",
                 [](const RetainedSpan& data)
                 {
                     const auto text = unwrap(data.utf8());
                     return nb::str(text.data(), text.size());
                 });

        type.def(
            "slice", [](const RetainedSpan& data, size_t offset, size_t length)
            {
                return unwrap(data.slice(offset, length));
            }, "offset"_a, "length"_a);
    }
}
