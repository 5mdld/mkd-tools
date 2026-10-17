//
// kiwakiwaaにより 2026/03/07 に作成されました。
//

#include "MKD/resource/keystore_writer.hpp"
#include "entry_id_codec.hpp"
#include "keystore_compare.hpp"
#include "keystore_format.hpp"
#include "binary/binary_buffer.hpp"
#include "io/build_directory.hpp"
#include "io/output_file.hpp"
#include "io/write_validation.hpp"

#include <algorithm>
#include <map>
#include <ranges>
#include <utility>

namespace MKD
{
    struct KeystoreWriter::Impl
    {
        struct Record
        {
            std::string_view key;
            std::vector<KeystorePosting> postings;
            SearchKeyOptions options;
            uint32_t offset = 0;

            Result<void> encode(detail::BinaryBuffer& words);
        };

        struct Layout
        {
            uint32_t wordsOffset;
            uint32_t indexOffset;
            uint32_t conversionOffset;
            uint32_t indexBytes;

            static Result<Layout> create(const KeystoreWriteOptions& options, size_t wordBytes, size_t entryCount);
        };

        std::unique_ptr<detail::BuildDirectory> build;
        KeystoreWriteOptions options;
        std::map<std::string, Record, std::less<>> records;
        size_t count = 0;

        Impl(std::unique_ptr<detail::BuildDirectory> build, const KeystoreWriteOptions& options)
            : build(std::move(build)), options(options)
        {
        }

        Result<void> encodeWords(detail::BinaryBuffer& words, std::vector<Record*>& entries);
        Result<void> writeHeader(detail::OutputFile& file, const Layout& layout) const;
        static Result<void> writeIndexes(detail::OutputFile& file, const Layout& layout,
                                         std::vector<Record*>& entries, detail::BinaryBuffer& buffer);
        Result<void> writeConversions(detail::OutputFile& file, detail::BinaryBuffer& buffer) const;
        Result<void> write();
    };

    KeystoreWriter::KeystoreWriter(std::unique_ptr<Impl> impl) : impl_(std::move(impl))
    {
    }

    KeystoreWriter::~KeystoreWriter() = default;
    KeystoreWriter::KeystoreWriter(KeystoreWriter&&) noexcept = default;
    KeystoreWriter& KeystoreWriter::operator=(KeystoreWriter&&) noexcept = default;

    Result<KeystoreWriter> KeystoreWriter::create(const std::filesystem::path& path,
                                                  const KeystoreWriteOptions& options)
    {
        if (options.version != KeystoreVersion::Version1 && options.version != KeystoreVersion::Version2)
            return std::unexpected("Unsupported keystore version");

        if (!options.conversions.empty() && options.version != KeystoreVersion::Version2)
            return std::unexpected("Key ID conversion requires keystore version 2");

        auto build = detail::BuildDirectory::create(path);
        if (!build)
            return std::unexpected(build.error());

        return KeystoreWriter(std::make_unique<Impl>(std::move(*build), options));
    }

    Result<void> KeystoreWriter::add(std::string_view key, const EntryId target, const SearchKeyOptions options) const
    {
        const KeystorePosting posting{target};
        return add(key, std::span(&posting, 1), options);
    }

    Result<void> KeystoreWriter::add(std::string_view key, std::span<const KeystorePosting> targets,
                                     const SearchKeyOptions options) const
    {
        if (auto state = impl_->build->check(); !state)
            return state;

        if (auto valid = detail::WriteValidation::text(key); !valid)
            return impl_->build->fail(valid.error());

        if (targets.empty())
            return impl_->build->fail("Search key must have at least one target");

        for (const auto& target : targets)
            if (target.entryId.pageId > 0xFFFFFF)
                return impl_->build->fail("Search target exceeds the 24-bit page ID limit");

        auto [it, inserted] = impl_->records.try_emplace(std::string(key), Impl::Record{{}, {}, options});
        if (inserted)
        {
            it->second.key = it->first;
            ++impl_->count;
        }

        if (!inserted && it->second.options != options)
            return impl_->build->fail("Conflicting options for the same search key");

        it->second.postings.insert(it->second.postings.end(), targets.begin(), targets.end());
        return {};
    }

    Result<void> KeystoreWriter::Impl::Record::encode(detail::BinaryBuffer& words)
    {
        std::ranges::sort(postings);
        auto duplicates = std::ranges::unique(postings);
        postings.erase(duplicates.begin(), duplicates.end());
        const bool wide = postings.size() > UINT16_MAX;
        const uint64_t maximum = words.position() + key.size() + 10 + uint64_t{9} * postings.size();
        if (maximum > UINT32_MAX)
            return std::unexpected("Keystore word section exceeds format limits");

        offset = static_cast<uint32_t>(words.position());
        words.writeLittle(static_cast<uint32_t>(words.position() + 6 + key.size()));
        words.writeByte(static_cast<uint8_t>((wide ? detail::keystore::word::wideCount : 0) |
                                                (options.exactOnly ? detail::keystore::word::exactOnly : 0)));
        words.writeBytes(std::span(reinterpret_cast<const uint8_t*>(key.data()), key.size() + 1));
        if (wide)
            words.writeLittle(static_cast<uint32_t>(postings.size()));
        else
            words.writeLittle(static_cast<uint16_t>(postings.size()));

        for (const auto& posting : postings)
            detail::keystore::encodeKeystoreEntry(words, posting.entryId, posting.dictionaryId, posting.type);

        return {};
    }

    Result<KeystoreWriter::Impl::Layout> KeystoreWriter::Impl::Layout::create(
        const KeystoreWriteOptions& options, const size_t wordBytes, const size_t entryCount)
    {
        const uint64_t headerBytes = options.version == KeystoreVersion::Version2 ? 32 : 16;
        const uint64_t indexBytes = 4 + uint64_t{4} * entryCount;
        const uint64_t conversionBytes = options.conversions.empty() ? 0 : 4 + uint64_t{8} * options.conversions.size();
        const uint64_t total = headerBytes + wordBytes + 20 + 3 * indexBytes + conversionBytes;
        if (total > UINT32_MAX)
            return std::unexpected("Keystore exceeds format limits");

        return Layout{
            .wordsOffset = static_cast<uint32_t>(headerBytes),
            .indexOffset = static_cast<uint32_t>(headerBytes + wordBytes),
            .conversionOffset = static_cast<uint32_t>(conversionBytes ? total - conversionBytes : 0),
            .indexBytes = static_cast<uint32_t>(indexBytes)
        };
    }

    Result<void> KeystoreWriter::Impl::encodeWords(detail::BinaryBuffer& words, std::vector<Record*>& entries)
    {
        entries.reserve(records.size());
        for (auto& record : records | std::views::values)
        {
            if (auto encoded = record.encode(words); !encoded)
                return encoded;
            entries.push_back(&record);
        }

        if (records.empty())
            words.writeByte(0);

        return {};
    }

    Result<void> KeystoreWriter::Impl::writeHeader(detail::OutputFile& file, const Layout& layout) const
    {
        detail::BinaryBuffer header;
        header.writeLittle(std::to_underlying(options.version));
        header.writeLittle(uint32_t{0});
        header.writeLittle(layout.wordsOffset);
        header.writeLittle(layout.indexOffset);
        if (options.version == KeystoreVersion::Version2)
        {
            header.writeLittle(layout.conversionOffset);
            header.writePadding(12);
        }
        return file.write(header.data());
    }

    Result<void> KeystoreWriter::Impl::writeIndexes(detail::OutputFile& file, const Layout& layout,
                                                    std::vector<Record*>& entries, detail::BinaryBuffer& buffer)
    {
        buffer.clear();
        buffer.writeLittle(uint32_t{4});
        for (unsigned i = 0; i < 3; ++i)
            buffer.writeLittle(static_cast<uint32_t>(20 + uint64_t{i} * layout.indexBytes));

        buffer.writeLittle(uint32_t{0});
        if (auto written = file.write(buffer.data()); !written)
            return written;

        for (auto mode : {
                 detail::keystore::CompareMode::Length, detail::keystore::CompareMode::Forward,
                 detail::keystore::CompareMode::Backward
             })
        {
            std::ranges::sort(entries, [mode](const Record* a, const Record* b)
            {
                const auto order = detail::keystore::compare(a->key, b->key, mode);
                return order == 0 ? a->key < b->key : order < 0;
            });
            buffer.clear();
            buffer.writeLittle(static_cast<uint32_t>(entries.size()));
            for (const auto record : entries)
                buffer.writeLittle(record->offset);

            if (auto written = file.write(buffer.data()); !written)
                return written;
        }
        return {};
    }

    Result<void> KeystoreWriter::Impl::writeConversions(detail::OutputFile& file, detail::BinaryBuffer& buffer) const
    {
        if (options.conversions.empty())
            return {};

        buffer.clear();
        buffer.writeLittle(static_cast<uint32_t>(options.conversions.size()));
        for (auto id : options.conversions)
        {
            buffer.writeLittle(id.pageId);
            buffer.writeLittle(id.itemId);
            buffer.writeLittle(uint16_t{0});
        }
        return file.write(buffer.data());
    }

    Result<void> KeystoreWriter::Impl::write()
    {
        detail::BinaryBuffer buffer;
        std::vector<Record*> entries;
        if (auto encoded = encodeWords(buffer, entries); !encoded)
            return encoded;

        auto layout = Layout::create(options, buffer.position(), entries.size());
        if (!layout)
            return std::unexpected(layout.error());

        detail::OutputFile file(build->path() / "data", false);
        if (!file.isOpen())
            return std::unexpected("Cannot create keystore file");

        if (auto written = writeHeader(file, *layout); !written)
            return written;

        if (auto written = file.write(buffer.data()); !written)
            return written;

        if (auto written = writeIndexes(file, *layout, entries, buffer); !written)
            return written;

        if (auto written = writeConversions(file, buffer); !written)
            return written;

        return file.finish().transform([](uint64_t)
        {
        });
    }

    Result<void> KeystoreWriter::finish() const
    {
        if (auto state = impl_->build->check(); !state)
            return state;

        if (auto written = impl_->write(); !written)
            return impl_->build->fail(written.error());

        auto published = impl_->build->publishFile("data");
        if (published)
            impl_->records = decltype(impl_->records){};

        return published;
    }

    size_t KeystoreWriter::size() const noexcept { return impl_->count; }
    bool KeystoreWriter::empty() const noexcept { return size() == 0; }
}
