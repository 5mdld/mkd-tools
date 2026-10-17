//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include <nanobind/nanobind.h>
#include <cstdint>
#include <span>

namespace MKD::Python
{
    class InputBytes
    {
    public:
        explicit InputBytes(nanobind::handle object);
        ~InputBytes();
        InputBytes(const InputBytes&) = delete;
        InputBytes& operator=(const InputBytes&) = delete;

        [[nodiscard]] std::span<const uint8_t> span() const;

    private:
        Py_buffer view_{};
    };
}
