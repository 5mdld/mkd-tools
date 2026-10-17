//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#pragma once

#include "MKD/resource/resource_store.hpp"

#include <filesystem>
#include <ostream>
#include <string>
#include <vector>
#include <variant>

namespace MKD
{
    /** A mapped font file or RSC fragments. Opening identifies the format without assembling the font. */
    class Font
    {
    public:
        static Result<Font> open(const std::filesystem::path& directoryPath);
        [[nodiscard]] const std::string& name() const noexcept;
        /** Resource name with the extension resolved from the font signature. */
        [[nodiscard]] const std::string& filename() const noexcept;
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
        /** Streams to a new file. Its parent directory must already exist. */
        [[nodiscard]] Result<uint64_t> writeTo(const std::filesystem::path& path) const;
        [[nodiscard]] bool empty() const noexcept;

    private:
        using Storage = std::variant<ResourceStore, RetainedSpan>;
        static Result<Font> create(std::string name, Storage fragments);
        Font(std::string name, Storage fragments);
        std::string name_;
        std::string filename_;
        Storage fragments_;
    };
}
