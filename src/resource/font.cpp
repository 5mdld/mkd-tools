//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#include "MKD/resource/font.hpp"
#include "resource/detail/font_format.hpp"

#include <algorithm>
#include <array>
#include <format>

namespace MKD
{
    Result<Font> Font::open(const std::filesystem::path& directoryPath)
    {
        const auto nameBytes = directoryPath.stem().u8string();
        const std::string name(reinterpret_cast<const char*>(nameBytes.data()), nameBytes.size());
        std::error_code error;
        const auto regular = std::filesystem::is_regular_file(directoryPath, error);
        if (error)
            return std::unexpected(error.message());

        if (regular)
        {
            auto mapping = RetainedSpan::map(directoryPath);
            if (!mapping)
                return std::unexpected(mapping.error());

            return Font(name, std::move(*mapping));
        }

        auto fragments = ResourceStore::open(directoryPath);
        if (!fragments)
            return std::unexpected(std::format("Failed to open font '{}': {}", directoryPath.string(),
                                               fragments.error()));

        return Font(name, std::move(*fragments));
    }

    Font::Font(std::string name, std::variant<ResourceStore, RetainedSpan> fragments)
        : name_(std::move(name)), fragments_(std::move(fragments))
    {
    }

    const std::string& Font::name() const noexcept { return name_; }

    size_t Font::fragmentCount() const noexcept
    {
        if (const auto store = std::get_if<ResourceStore>(&fragments_))
            return store->size();
        return 1;
    }

    bool Font::empty() const noexcept
    {
        return std::visit([](const auto& fragments) { return fragments.empty(); }, fragments_);
    }

    Result<RetainedSpan> Font::fragmentAt(const size_t index) const
    {
        if (const auto store = std::get_if<ResourceStore>(&fragments_))
            return store->itemAt(index).transform([](ResourceStoreItem item)
            {
                return std::move(item.data);
            });

        if (index != 0)
            return std::unexpected("Font fragment index is out of range");

        return std::get<RetainedSpan>(fragments_);
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

        const auto extension = detail::FontFormat::detect(std::span(signature).first(length));
        if (!extension)
            return std::optional<std::string>{};

        return std::optional<std::string>(extension->substr(1));
    }
}
