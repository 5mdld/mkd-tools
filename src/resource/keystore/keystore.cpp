//
// kiwakiwaaにより 2026/02/11 に作成されました。
//

#include "MKD/resource/keystore.hpp"
#include "MKD/platform/mmap_file.hpp"
#include "binary/binary_view.hpp"
#include "entry_id_codec.hpp"

#include <array>
#include <format>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace MKD
{
    struct Keystore::Impl
    {
        struct WordEntry
        {
            std::string_view key;
            uint32_t pagesOffset;
            uint8_t flags;
        };

        struct Layout
        {
            detail::BinaryView words;
            detail::BinaryView indices;
            std::optional<detail::BinaryView> conversions;
        };

        static Result<uint32_t> readVersion2Header(detail::BinaryView data);
        static Result<Layout> readLayout(detail::BinaryView data);
        static Result<std::array<detail::BinaryView, 4>> readIndices(detail::BinaryView section);
        static Result<detail::BinaryView> readConversions(std::optional<detail::BinaryView> section);
        static Result<std::unique_ptr<Impl>> read(MappedFile file, std::string filename);

        MappedFile file;
        std::string filename;
        std::optional<KeystoreScope> scope;
        detail::BinaryView words;
        std::array<detail::BinaryView, 4> indices;
        detail::BinaryView conversions;
        mutable std::once_flag inverseInit;
        mutable Result<void> inverseStatus;
        mutable std::unordered_map<EntryId, std::vector<size_t>> inverse;

        Impl(MappedFile file, std::string filename, detail::BinaryView words)
            : file(std::move(file)), filename(std::move(filename)),
              scope(parseKeystoreScopeFilename(this->filename)), words(words)
        {
        }

        static Result<detail::BinaryView> parseIndex(detail::BinaryView section, const size_t start, const size_t end)
        {
            if (start == 0)
                return detail::BinaryView{};
            if (start < 20 || end <= start)
                return std::unexpected("Invalid keystore index range");

            auto data = section.slice(start, end - start);
            if (!data)
                return std::unexpected(data.error());

            auto count = data->readLittle<uint32_t>(0);
            if (!count)
                return std::unexpected(count.error());

            auto offsets = data->table(4, *count, 4);
            if (!offsets || offsets->bytes().size() != data->bytes().size() - 4)
                return std::unexpected("Keystore index count does not match its size");

            return offsets;
        }

        [[nodiscard]] Result<uint32_t> wordOffset(KeystoreIndex type, const size_t index) const
        {
            const auto position = static_cast<size_t>(type);
            if (position >= indices.size())
                return std::unexpected("Invalid keystore index type");

            if (index >= indices[position].bytes().size() / 4)
                return std::unexpected("Keystore index out of bounds");

            return indices[position].readLittle<uint32_t>(index * 4);
        }

        [[nodiscard]] Result<WordEntry> parseWordEntry(const uint32_t offset) const
        {
            auto word = words.tail(offset);
            if (!word)
                return std::unexpected(word.error());

            auto header = word->slice(0, 5);
            if (!header)
                return std::unexpected(header.error());

            auto key = word->nullTerminatedString(5);
            if (!key)
                return std::unexpected(key.error());

            return WordEntry{
                .key = *key,
                .pagesOffset = *header->readLittle<uint32_t>(0),
                .flags = *header->readLittle<uint8_t>(4),
            };
        }

        [[nodiscard]] Result<std::vector<EntryId>> decodeIds(const WordEntry& word) const
        {
            auto data = words.tail(word.pagesOffset);
            if (!data)
                return std::unexpected(data.error());

            auto ids = detail::keystore::decodeEntryIds(data->bytes(), (word.flags & 0x04) != 0);
            if (!ids)
                return std::unexpected(ids.error());

            for (auto& id : *ids)
                if (id.pageId < conversions.bytes().size() / 8)
                {
                    const size_t offset = static_cast<size_t>(id.pageId) * 8;
                    id = {*conversions.readLittle<uint32_t>(offset), *conversions.readLittle<uint16_t>(offset + 4)};
                }
            return ids;
        }

        Result<void> buildInverse() const
        {
            const auto& prefix = indices[static_cast<size_t>(KeystoreIndex::Prefix)];
            for (size_t i = 0; i < prefix.bytes().size() / 4; ++i)
            {
                auto word = parseWordEntry(*prefix.readLittle<uint32_t>(i * 4));
                if (!word)
                    return std::unexpected(word.error());

                auto ids = decodeIds(*word);
                if (!ids)
                    return std::unexpected(ids.error());

                std::unordered_set<EntryId> seen;
                for (const auto& id : *ids)
                    if (seen.insert(id).second)
                        inverse[id].push_back(i);
            }
            return {};
        }
    };

    Keystore::Keystore(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl))
    {
    }

    Keystore::~Keystore() = default;
    Keystore::Keystore(Keystore&&) noexcept = default;
    Keystore& Keystore::operator=(Keystore&&) noexcept = default;

    Result<uint32_t> Keystore::Impl::readVersion2Header(detail::BinaryView data)
    {
        auto extension = data.slice(16, 16);
        if (!extension)
            return std::unexpected(extension.error());

        for (size_t offset = 4; offset < 16; offset += 4)
            if (*extension->readLittle<uint32_t>(offset) != 0)
                return std::unexpected("Invalid keystore v2 magic");

        return extension->readLittle<uint32_t>(0);
    }

    Result<Keystore::Impl::Layout> Keystore::Impl::readLayout(detail::BinaryView data)
    {
        auto header = data.slice(0, 16);
        if (!header)
            return std::unexpected(header.error());

        const auto version = *header->readLittle<uint32_t>(0);
        if (version != detail::keystore::version1 && version != detail::keystore::version2)
            return std::unexpected(std::format("Invalid keystore version: 0x{:x}", version));

        if (*header->readLittle<uint32_t>(4) != 0)
            return std::unexpected("Invalid keystore magic");

        const bool version2 = version == detail::keystore::version2;
        auto conversionOffset = version2 ? readVersion2Header(data) : Result<uint32_t>(0);
        if (!conversionOffset)
            return std::unexpected(conversionOffset.error());

        const size_t indexEnd = *conversionOffset == 0 ? data.bytes().size() : *conversionOffset;
        const auto wordsOffset = *header->readLittle<uint32_t>(8);
        const auto indexOffset = *header->readLittle<uint32_t>(12);
        if (wordsOffset < (version2 ? 32u : 16u) || wordsOffset >= indexOffset || indexOffset >= indexEnd)
            return std::unexpected("Invalid keystore section ordering");

        auto words = data.slice(wordsOffset, indexOffset - wordsOffset);
        auto indices = data.slice(indexOffset, indexEnd - indexOffset);
        if (!words || !indices)
            return std::unexpected("Keystore section exceeds file size");

        std::optional<detail::BinaryView> conversions;
        if (*conversionOffset != 0)
        {
            auto section = data.tail(*conversionOffset);
            if (!section)
                return std::unexpected(section.error());
            conversions = *section;
        }
        return Layout{.words = *words, .indices = *indices, .conversions = conversions};
    }

    Result<std::array<detail::BinaryView, 4>> Keystore::Impl::readIndices(detail::BinaryView section)
    {
        auto header = section.slice(0, 20);
        if (!header)
            return std::unexpected(header.error());

        if (*header->readLittle<uint32_t>(0) != 4)
            return std::unexpected("Invalid keystore index magic");

        std::array<detail::BinaryView, 4> indices;
        size_t end = section.bytes().size();
        // Absent arrays do not move the boundary of the preceding array.
        for (size_t i = indices.size(); i-- > 0;)
        {
            const auto start = *header->readLittle<uint32_t>(4 + i * 4);
            auto index = parseIndex(section, start, end);
            if (!index)
                return std::unexpected(std::format("Keystore index {}: {}", i, index.error()));
            indices[i] = std::move(*index);
            if (start != 0)
                end = start;
        }
        return indices;
    }

    Result<detail::BinaryView> Keystore::Impl::readConversions(std::optional<detail::BinaryView> section)
    {
        if (!section)
            return detail::BinaryView{};

        auto count = section->readLittle<uint32_t>(0);
        if (!count)
            return std::unexpected(count.error());

        return section->table(4, *count, 8);
    }

    Result<std::unique_ptr<Keystore::Impl>> Keystore::Impl::read(MappedFile file, std::string filename)
    {
        auto layout = readLayout(detail::BinaryView(file.data()));
        if (!layout)
            return std::unexpected(layout.error());

        auto indices = readIndices(layout->indices);
        if (!indices)
            return std::unexpected(indices.error());

        auto conversions = readConversions(layout->conversions);
        if (!conversions)
            return std::unexpected(conversions.error());

        auto impl = std::make_unique<Impl>(std::move(file), std::move(filename), layout->words);
        impl->indices = std::move(*indices);
        impl->conversions = std::move(*conversions);
        return impl;
    }

    Result<Keystore> Keystore::open(const std::filesystem::path& path)
    {
        auto file = MappedFile::open(path);
        if (!file)
            return std::unexpected(file.error());

        auto impl = Impl::read(std::move(*file), path.filename().string());
        if (!impl)
            return std::unexpected(impl.error());

        return Keystore(std::move(*impl));
    }

    Result<KeystoreEntry> Keystore::entryAt(const KeystoreIndex type, const size_t index) const
    {
        auto offset = impl_->wordOffset(type, index);
        if (!offset)
            return std::unexpected(offset.error());

        auto word = impl_->parseWordEntry(*offset);
        if (!word)
            return std::unexpected(word.error());

        return KeystoreEntry{.key = word->key, .index = index, .flags = word->flags};
    }

    Result<std::string_view> Keystore::keyAt(const KeystoreIndex type, const size_t index) const
    {
        auto entry = entryAt(type, index);
        if (!entry)
            return std::unexpected(entry.error());
        return entry->key;
    }

    Result<std::vector<EntryId>> Keystore::entryIdsAt(const KeystoreIndex type, const size_t index) const
    {
        auto offset = impl_->wordOffset(type, index);
        if (!offset)
            return std::unexpected(offset.error());

        auto word = impl_->parseWordEntry(*offset);
        if (!word)
            return std::unexpected(word.error());

        return impl_->decodeIds(*word);
    }

    size_t Keystore::indexSize(KeystoreIndex type) const noexcept
    {
        const auto position = static_cast<size_t>(type);
        return position < 4 ? impl_->indices[position].bytes().size() / 4 : 0;
    }

    std::string_view Keystore::filename() const noexcept { return impl_->filename; }
    std::optional<KeystoreScope> Keystore::scope() const noexcept { return impl_->scope; }

    Result<std::vector<std::string_view>> Keystore::keysForEntry(const EntryId& id) const
    {
        std::call_once(impl_->inverseInit, [this] { impl_->inverseStatus = impl_->buildInverse(); });
        if (!impl_->inverseStatus)
            return std::unexpected(impl_->inverseStatus.error());

        std::vector<std::string_view> keys;
        if (const auto hit = impl_->inverse.find(id); hit != impl_->inverse.end())
        {
            std::unordered_set<std::string_view> seen;
            for (const auto index : hit->second)
            {
                auto key = keyAt(KeystoreIndex::Prefix, index);
                if (!key)
                    return std::unexpected(key.error());
                if (seen.insert(*key).second)
                    keys.push_back(*key);
            }
        }
        return keys;
    }
}
