//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#pragma once

#include "MKD/resource/resource_store.hpp"

#include <filesystem>
#include <ostream>
#include <optional>
#include <string>
#include <vector>
#include <variant>

namespace MKD
{
    /** A mapped font file or RSC fragments. Opening does not assemble the font. */
    class Font
    {
    public:
        static Result<Font> open(const std::filesystem::path& directoryPath);
        [[nodiscard]] const std::string& name() const noexcept;
        [[nodiscard]] size_t fragmentCount() const noexcept;
        [[nodiscard]] Result<RetainedSpan> fragmentAt(size_t index) const;

        [[nodiscard]] auto fragments() const &
        {
            return std::views::iota(size_t{0}, fragmentCount())
                | std::views::transform([this](const size_t index) { return fragmentAt(index); });
        }

        auto fragments() const && = delete;

        /** Explicitly assemble a contiguous copy when the caller needs one. */
        [[nodiscard]] Result<std::vector<uint8_t>> readAll() const;
        [[nodiscard]] Result<uint64_t> writeTo(std::ostream& output) const;
        [[nodiscard]] Result<std::optional<std::string>> detectType() const;
        [[nodiscard]] bool empty() const noexcept;

    private:
        Font(std::string name, std::variant<ResourceStore, RetainedSpan> fragments);
        std::string name_;
        std::variant<ResourceStore, RetainedSpan> fragments_;
    };
}
