//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#include "MKD/resource/font.hpp"
#include "binary/binary_view.hpp"

#include <algorithm>
#include <array>
#include <format>

namespace MKD
{
    Result<Font> Font::open(const std::filesystem::path& directoryPath)
    {
        auto fragments = ResourceStore::open(directoryPath);
        if (!fragments)
            return std::unexpected(std::format("Failed to open font '{}': {}", directoryPath.string(),
                                               fragments.error()));
        return Font(directoryPath.stem().string(), std::move(*fragments));
    }

    Font::Font(std::string name, ResourceStore fragments)
        : name_(std::move(name)), fragments_(std::move(fragments))
    {
    }

    const std::string& Font::name() const noexcept { return name_; }
    size_t Font::fragmentCount() const noexcept { return fragments_.size(); }
    bool Font::empty() const noexcept { return fragments_.empty(); }

    Result<RetainedSpan> Font::fragmentAt(const size_t index) const
    {
        return fragments_.itemAt(index).transform([](ResourceStoreItem item) { return std::move(item.data); });
    }

    Result<std::vector<uint8_t>> Font::readAll() const
    {
        std::vector<uint8_t> bytes;
        for (auto fragment : fragments())
        {
            if (!fragment)
                return std::unexpected(fragment.error());
            bytes.insert(bytes.end(), fragment->begin(), fragment->end());
        }
        return bytes;
    }

    Result<uint64_t> Font::writeTo(std::ostream& output) const
    {
        uint64_t written = 0;
        for (auto fragment : fragments())
        {
            if (!fragment)
                return std::unexpected(fragment.error());
            output.write(reinterpret_cast<const char*>(fragment->data()),
                         static_cast<std::streamsize>(fragment->size()));
            if (!output)
                return std::unexpected("Failed to write font fragment");
            written += fragment->size();
        }
        return written;
    }

    Result<std::optional<std::string>> Font::detectType() const
    {
        std::array<uint8_t, 4> signature{};
        size_t length = 0;
        for (auto fragment : fragments())
        {
            if (!fragment)
                return std::unexpected(fragment.error());
            const size_t count = std::min(signature.size() - length, fragment->size());
            std::copy_n(fragment->begin(), count, signature.begin() + length);
            length += count;
            if (length == signature.size())
                break;
        }
        if (length < signature.size())
            return std::optional<std::string>{};
        const auto magic = *detail::BinaryView(signature).readBig<uint32_t>(0);
        if (magic == 0x4F54544F)
            return std::optional<std::string>("otf");
        if (magic == 0x00010000 || magic == 0x74727565)
            return std::optional<std::string>("ttf");
        return std::optional<std::string>{};
    }
}
