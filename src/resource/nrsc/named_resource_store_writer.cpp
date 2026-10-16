//
// kiwakiwaaにより 2026/03/07 に作成されました。
//

#include "named_resource_store_writer.hpp"
#include "binary/binary_buffer.hpp"
#include "named_resource_store_index.hpp"
#include "resource/detail/zlib.hpp"

#include <algorithm>
#include <numeric>
#include <limits>

namespace MKD
{
    Result<NamedResourceStoreWriter> NamedResourceStoreWriter::create(const std::filesystem::path& directoryPath,
                                                                      const Options& options)
    {
        if (options.type != ResourceType::Audio && options.type != ResourceType::Graphics)
            return std::unexpected(std::format("Resource type '{}' is not supported by nrsc",
                                               resourceTypeName(options.type)));

        if (options.compressionLevel < -1 || options.compressionLevel > 9)
            return std::unexpected("Invalid default compression level");
        auto blobWriter = detail::SequentialBlobWriter::create(
            directoryPath, options.type, options.maxFileSize);

        if (!blobWriter)
            return std::unexpected(blobWriter.error());

        return NamedResourceStoreWriter(std::move(*blobWriter), options.compressionLevel);
    }


    NamedResourceStoreWriter::NamedResourceStoreWriter(detail::SequentialBlobWriter&& blobWriter, int compressionLevel)
        : blobWriter_(std::move(blobWriter)), compressionLevel_(compressionLevel)
    {
    }


    Result<void> NamedResourceStoreWriter::add(std::string_view id, std::vector<uint8_t> data,
                                               std::optional<int> compressionLevel)
    {
        const int level = compressionLevel.value_or(compressionLevel_);
        if (finished_)
            return std::unexpected("Cannot add entries after finalize()");

        if (id.empty() || id.contains('\0'))
            return std::unexpected("Entry ID must be nonempty and contain no null bytes");

        if (level < -1 || level > 9)
            return std::unexpected("Invalid compression level");
        const bool compress = level != 0;
        std::span<const uint8_t> dataToWrite;
        std::vector<uint8_t> compressedData;

        if (compress)
        {
            auto compressed = detail::zlib::compress(data, level);
            if (!compressed)
                return std::unexpected(compressed.error());

            compressedData = std::move(*compressed);
            dataToWrite = compressedData;
        }
        else
        {
            dataToWrite = data;
        }

        auto location = blobWriter_.write(dataToWrite);
        if (!location)
            return std::unexpected(location.error());
        if (location->sequence > std::numeric_limits<uint16_t>::max() ||
            location->fileOffset > std::numeric_limits<uint32_t>::max())
            return std::unexpected("Named resource location exceeds format limits");

        entries_.push_back({
            .id = std::string(id),
            .format = static_cast<uint16_t>(compress ? 1u : 0u),
            .fileSequence = static_cast<uint16_t>(location->sequence),
            .fileOffset = static_cast<uint32_t>(location->fileOffset),
            .length = static_cast<uint32_t>(dataToWrite.size()),
        });

        return {};
    }


    Result<void> NamedResourceStoreWriter::finalize()
    {
        if (finished_)
            return std::unexpected("finalize() already called");

        if (auto result = blobWriter_.finalize(); !result)
            return std::unexpected(result.error());

        if (auto result = writeIndexFile(); !result)
            return std::unexpected(result.error());

        finished_ = true;
        return {};
    }


    size_t NamedResourceStoreWriter::size() const noexcept
    {
        return entries_.size();
    }


    bool NamedResourceStoreWriter::empty() const noexcept
    {
        return entries_.empty();
    }


    Result<void> NamedResourceStoreWriter::writeIndexFile() const
    {
        const auto directory = blobWriter_.directory();
        const auto idxPath = directory / "index.nidx";

        // build sorted order by ID for binary search when reading
        std::vector<size_t> sortedOrder(entries_.size());
        std::iota(sortedOrder.begin(), sortedOrder.end(), 0);
        std::ranges::sort(sortedOrder, [&](const size_t a, const size_t b)
        {
            return entries_[a].id < entries_[b].id;
        });

        for (size_t i = 1; i < sortedOrder.size(); ++i)
            if (entries_[sortedOrder[i - 1]].id == entries_[sortedOrder[i]].id)
                return std::unexpected("Duplicate named resource ID");

        const uint64_t indexSize = 8 + uint64_t{16} * entries_.size();
        uint64_t stringsSize = 0;
        for (const auto& entry : entries_)
            stringsSize += entry.id.size() + 1;
        if (indexSize + stringsSize > std::numeric_limits<uint32_t>::max())
            return std::unexpected("Named resource index exceeds format limits");

        // build records array in sorted order
        std::string idStrings;
        std::vector<NamedResourceStoreIndexRecord> records;
        records.reserve(entries_.size());

        for (const size_t idx : sortedOrder)
        {
            const auto& [id, format, fileSequence, fileOffset, length] = entries_[idx];
            const auto idOffset = static_cast<uint32_t>(idStrings.size());
            idStrings.append(id);
            idStrings.push_back('\0');

            NamedResourceStoreIndexRecord record{
                .format = format,
                .fileSequence = fileSequence,
                .idStringOffset = idOffset,
                .fileOffset = fileOffset,
                .length = length
            };
            records.push_back(record);
        }

        // build the ID strings region
        const size_t headerSize = HEADER_SIZE + records.size() * RECORD_SIZE;
        for (auto& record : records)
            record.idStringOffset += static_cast<uint32_t>(headerSize);

        std::ofstream file(idxPath, std::ios::binary | std::ios::trunc);
        if (!file)
            return std::unexpected(std::format("Failed to open: {}", idxPath.string()));

        NamedResourceStoreIndexHeader header{
            .zeroField = 0,
            .recordCount = static_cast<uint32_t>(records.size())
        };

        detail::BinaryBuffer buffer;
        buffer.writeLittle(header.zeroField);
        buffer.writeLittle(header.recordCount);
        for (const auto& record : records)
        {
            buffer.writeLittle(record.format);
            buffer.writeLittle(record.fileSequence);
            buffer.writeLittle(record.idStringOffset);
            buffer.writeLittle(record.fileOffset);
            buffer.writeLittle(record.length);
        }
        file.write(reinterpret_cast<const char*>(buffer.data().data()),
                   static_cast<std::streamsize>(buffer.position()));
        file.write(idStrings.data(), static_cast<std::streamsize>(idStrings.size()));

        if (!file)
            return std::unexpected(std::format("Failed to write: {}", idxPath.string()));

        return {};
    }
}
