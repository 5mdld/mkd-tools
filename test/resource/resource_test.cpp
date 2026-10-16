//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#include <gtest/gtest.h>

#include "MKD/mkd.hpp"
#include "binary/binary_view.hpp"
#include "binary/binary_buffer.hpp"
#include "resource/detail/resource_files.hpp"
#include "resource/detail/zlib.hpp"
#include "resource/rsc/resource_store_index.hpp"
#include "resource/rsc/resource_store_writer.hpp"
#include "resource/nrsc/named_resource_store_index.hpp"
#include "resource/nrsc/named_resource_store_writer.hpp"
#include "resource/keystore/keystore_writer.hpp"

#include <array>
#include <chrono>
#include <fstream>
#include <limits>
#include <random>
#include <sstream>
#include <unordered_set>

namespace MKD::test
{
    class ResourceFormatTest : public ::testing::Test
    {
    protected:
        std::filesystem::path directory;

        void SetUp() override
        {
            directory = std::filesystem::temp_directory_path() /
                ("mkd-resources-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                 "-" + std::to_string(std::random_device{}()));
            std::filesystem::create_directories(directory);
        }

        void TearDown() override
        {
            std::error_code ec;
            std::filesystem::remove_all(directory, ec);
        }

        void write(std::string_view name, std::span<const uint8_t> bytes)
        {
            std::ofstream file(directory / name, std::ios::binary);
            file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            ASSERT_TRUE(file);
        }

        void words(std::string_view name, std::initializer_list<uint32_t> values)
        {
            detail::BinaryBuffer bytes;
            for (auto value : values)
                bytes.writeLittle(value);
            write(name, bytes.data());
        }

        void directStore()
        {
            words("contents.map", {0, 2, 0, 0xFFFFFFFF, 5, 0xFFFFFFFF});
            words("contents.idx", {2, 0, 10, 1, 20, 0});
            const std::array<uint8_t, 10> data{1, 0, 0, 0, 'a', 1, 0, 0, 0, 'b'};
            write("contents-0001.rsc", data);
        }
    };

    TEST_F(ResourceFormatTest, ReadsUnalignedIntegersInBothByteOrders)
    {
        const std::array<uint8_t, 9> bytes{0, 1, 2, 3, 4, 5, 6, 7, 8};
        const detail::BinaryView view(bytes);
        EXPECT_EQ(*view.readLittle<uint16_t>(1), 0x0201);
        EXPECT_EQ(*view.readBig<uint16_t>(1), 0x0102);
        EXPECT_EQ(*view.readLittle<uint32_t>(1), 0x04030201u);
        EXPECT_EQ(*view.readBig<uint32_t>(1), 0x01020304u);
        EXPECT_EQ((*view.readBig<uint32_t, 3>(1)), 0x010203u);
        EXPECT_EQ((*view.readLittle<uint32_t, 3>(1)), 0x030201u);
        EXPECT_EQ(*view.readBig<uint64_t>(1), 0x0102030405060708ull);
        EXPECT_EQ(*view.readLittle<uint64_t>(1), 0x0807060504030201ull);
    }

    TEST_F(ResourceFormatTest, ChecksRangesAndTableArithmeticBeforeReading)
    {
        const std::array<uint8_t, 4> bytes{1, 2, 3, 4};
        const detail::BinaryView view(bytes);
        EXPECT_FALSE(view.readLittle<uint32_t>(1));
        EXPECT_FALSE(view.slice(std::numeric_limits<size_t>::max(), 2));
        EXPECT_FALSE(view.slice(1, std::numeric_limits<size_t>::max()));
        EXPECT_FALSE(view.table(0, std::numeric_limits<size_t>::max(), 8));
        EXPECT_FALSE(view.table(0, 1, 0));
        EXPECT_TRUE(view.slice(4, 0));
        EXPECT_FALSE(view.nullTerminatedString(0));
        EXPECT_FALSE(view.tail(5));
        write("bytes", bytes);
        auto mapping = MappedFile::open(directory / "bytes");
        ASSERT_TRUE(mapping);
        EXPECT_FALSE(mapping->slice(std::numeric_limits<size_t>::max(), 2));
    }

    TEST_F(ResourceFormatTest, SerialisesExplicitByteOrdersWithoutStructLayout)
    {
        detail::BinaryBuffer bytes;
        bytes.writeLittle(uint16_t{0x1234});
        bytes.writeBig(uint32_t{0x12345678});
        bytes.writeBig<uint32_t, 3>(0xabcdef);
        const std::vector<uint8_t> expected{0x34, 0x12, 0x12, 0x34, 0x56, 0x78, 0xab, 0xcd, 0xef};
        EXPECT_EQ(bytes.take(), expected);
        EXPECT_TRUE(bytes.data().empty());
        bytes.writeByte(42);
        EXPECT_EQ(bytes.take(), std::vector<uint8_t>{42});
    }

    TEST_F(ResourceFormatTest, RscPositionsResolveThroughTheMapIndex)
    {
        directStore();
        auto store = ResourceStore::open(directory);
        ASSERT_TRUE(store) << store.error();
        EXPECT_EQ(store->size(), 2);
        EXPECT_EQ(*store->idAt(0), 10);
        EXPECT_EQ(*store->indexOf(10), 0);
        EXPECT_EQ(*store->get(10)->utf8(), "b");
        EXPECT_EQ(*store->itemAt(0)->data.utf8(), "b");
        EXPECT_EQ(*store->itemAt(1)->data.utf8(), "a");
        EXPECT_FALSE(store->itemAt(2));
        EXPECT_FALSE(store->indexOf(999));
    }

    TEST_F(ResourceFormatTest, RejectsInvalidAndUnsortedRscMappings)
    {
        directStore();
        words("contents.idx", {1, 0, 10, 2});
        EXPECT_FALSE(ResourceStore::open(directory));
        words("contents.idx", {2, 0, 20, 0, 10, 1});
        EXPECT_FALSE(ResourceStore::open(directory));
        words("contents.idx", {2, 0, 10, 0, 10, 1});
        EXPECT_FALSE(ResourceStore::open(directory));
        words("contents.map", {0, 0xFFFFFFFF});
        EXPECT_FALSE(ResourceStore::open(directory));
    }

    TEST_F(ResourceFormatTest, MappedPayloadAndItsSlicesOutliveTheStore)
    {
        directStore();
        RetainedSpan payload;
        {
            auto store = ResourceStore::open(directory);
            ASSERT_TRUE(store);
            payload = *store->get(10);
        }
        auto slice = payload.slice(0, 1);
        ASSERT_TRUE(slice);
        payload = RetainedSpan{};
        EXPECT_EQ(*slice->utf8(), "b");
        EXPECT_FALSE(slice->slice(1, 1));
    }

    TEST_F(ResourceFormatTest, SequentialWriterMatchesTheLengthPrefixedWireFormat)
    {
        const std::array<uint8_t, 2> a{'a', 'b'};
        auto writer = ResourceStoreWriter::createSequential(directory, {.type = ResourceType::Contents});
        ASSERT_TRUE(writer);
        ASSERT_TRUE(writer->addData(a));
        ASSERT_TRUE(writer->addData({}));
        ASSERT_TRUE(writer->finalize());
        auto mapping = MappedFile::open(directory / "contents-0001.rsc");
        ASSERT_TRUE(mapping);
        const std::vector<uint8_t> expected{2, 0, 0, 0, 'a', 'b', 0, 0, 0, 0};
        EXPECT_EQ(std::vector<uint8_t>(mapping->data().begin(), mapping->data().end()), expected);
        auto store = ResourceStore::open(directory);
        ASSERT_TRUE(store) << store.error();
        EXPECT_EQ(store->size(), 2);
        EXPECT_EQ(*store->get(0)->utf8(), "ab");
        EXPECT_TRUE(store->get(1)->empty());
    }

    TEST_F(ResourceFormatTest, IndexedWriterPreservesIdsAcrossChunksAndFiles)
    {
        auto writer = ResourceStoreWriter::createIndexed(directory,
            {.type = ResourceType::Contents, .maxFileSize = 1, .chunkSize = 1,
             .useEncryption = true, .dictId = "fixture"});
        ASSERT_TRUE(writer);
        ASSERT_TRUE(writer->add(20, {'a'}));
        ASSERT_TRUE(writer->add(10, {'b'}));
        ASSERT_TRUE(writer->finalize());
        RetainedSpan payload;
        {
            auto store = ResourceStore::open(directory, "fixture");
            ASSERT_TRUE(store) << store.error();
            EXPECT_EQ(*store->idAt(0), 10);
            EXPECT_EQ(*store->itemAt(0)->data.utf8(), "b");
            EXPECT_EQ(*store->get(20)->utf8(), "a");
            payload = *store->get(10);
        }
        EXPECT_EQ(*payload.utf8(), "b");
    }

    TEST_F(ResourceFormatTest, LazyItemRangesCarryPayloadErrors)
    {
        directStore();
        words("contents.map", {0, 2, 999, 0xFFFFFFFF, 5, 0xFFFFFFFF});
        auto store = ResourceStore::open(directory);
        ASSERT_TRUE(store);
        size_t failures = 0;
        size_t successes = 0;
        for (auto item : store->items())
            item ? ++successes : ++failures;
        EXPECT_EQ(failures, 1);
        EXPECT_EQ(successes, 1);
    }

    TEST_F(ResourceFormatTest, NamedItemsRetainTheirIdsAndBothPayloadRepresentations)
    {
        auto writer = NamedResourceStoreWriter::create(directory, {.type = ResourceType::Graphics});
        ASSERT_TRUE(writer);
        ASSERT_TRUE(writer->add("plain", {'a'}));
        ASSERT_TRUE(writer->add("compressed", std::vector<uint8_t>(1000, 'b'), 9));
        ASSERT_TRUE(writer->finalize());
        std::vector<NamedResourceStoreItem> items;
        {
            auto store = NamedResourceStore::open(directory);
            ASSERT_TRUE(store) << store.error();
            EXPECT_EQ(*store->indexOf("plain"), 1);
            EXPECT_EQ(*store->idAt(0), "compressed");
            for (auto item : store->items())
            {
                ASSERT_TRUE(item);
                items.push_back(std::move(*item));
            }
        }
        ASSERT_EQ(items.size(), 2);
        EXPECT_EQ(items[0].id, "compressed");
        EXPECT_EQ(items[0].data.size(), 1000);
        EXPECT_EQ(*items[1].data.utf8(), "a");
    }

    TEST_F(ResourceFormatTest, MalformedNamedStringOffsetsReturnErrorsWithoutThrowing)
    {
        words("index.nidx", {0, 1, 0, 0, 0, 0});
        EXPECT_NO_THROW(EXPECT_FALSE(NamedResourceStoreIndex::open(directory)));
        words("index.nidx", {0, 1, 0, 0xFFFFFFFF, 0, 0});
        EXPECT_NO_THROW(EXPECT_FALSE(NamedResourceStoreIndex::open(directory)));
        words("index.nidx", {0, 1, 0, 24, 0, 0});
        EXPECT_FALSE(NamedResourceStoreIndex::open(directory));
    }

    TEST_F(ResourceFormatTest, BlobDiscoveryRejectsGapsDuplicatesAndPartialNumbers)
    {
        words("00001.nrsc", {0});
        EXPECT_FALSE(detail::ResourceFiles::open(directory, ".nrsc"));
        words("00000.nrsc", {0});
        EXPECT_TRUE(detail::ResourceFiles::open(directory, ".nrsc"));
        words("0.nrsc", {0});
        EXPECT_FALSE(detail::ResourceFiles::open(directory, ".nrsc"));
        std::filesystem::remove(directory / "0.nrsc");
        words("2junk.nrsc", {0});
        EXPECT_FALSE(detail::ResourceFiles::open(directory, ".nrsc"));
        EXPECT_FALSE(detail::resourcePaths(directory / "missing", ".nrsc"));
    }

    TEST_F(ResourceFormatTest, DecompressionReturnsExactBytesForAnyInitialSize)
    {
        const std::vector<uint8_t> input(1000, 'a');
        auto compressed = detail::zlib::compress(input);
        ASSERT_TRUE(compressed);
        for (size_t hint : {size_t{0}, size_t{1}, size_t{2048}})
        {
            auto decompressed = detail::zlib::decompress(*compressed, hint);
            ASSERT_TRUE(decompressed) << decompressed.error();
            EXPECT_EQ(*decompressed, input);
        }
        compressed->pop_back();
        EXPECT_FALSE(detail::zlib::decompress(*compressed));
    }

    TEST_F(ResourceFormatTest, EmptyCompressedStreamIsValidButEmptyInputIsNot)
    {
        auto compressed = detail::zlib::compress({});
        ASSERT_TRUE(compressed);
        auto decompressed = detail::zlib::decompress(*compressed, 0);
        ASSERT_TRUE(decompressed);
        EXPECT_TRUE(decompressed->empty());
        EXPECT_FALSE(detail::zlib::decompress({}));
    }

    TEST_F(ResourceFormatTest, RejectsIncompleteEncryptedPermutationBlocks)
    {
        const std::array<uint8_t, 5> bytes{0, 0xb4, 0xa2, 0xd9, 0xfb};
        EXPECT_FALSE(ResourceStoreCrypto::decrypt(bytes, {}));
        const std::array<uint8_t, 4> empty{0xb4, 0xa2, 0xd9, 0xfb};
        auto decoded = ResourceStoreCrypto::decrypt(empty, {});
        ASSERT_TRUE(decoded);
        EXPECT_TRUE(decoded->empty());
    }

    TEST_F(ResourceFormatTest, MovingARetainedSpanLeavesAnEmptySource)
    {
        RetainedSpan original(std::vector<uint8_t>{'a'});
        RetainedSpan moved(std::move(original));
        EXPECT_TRUE(original.empty());
        RetainedSpan assigned;
        assigned = std::move(moved);
        EXPECT_TRUE(moved.empty());
        EXPECT_EQ(*assigned.utf8(), "a");
        RetainedSpan invalid(std::vector<uint8_t>{0xff});
        EXPECT_FALSE(invalid.utf8());
    }

    TEST_F(ResourceFormatTest, EmptyIndexedPayloadsHaveAnUnambiguousHeader)
    {
        auto writer = ResourceStoreWriter::createIndexed(directory, {.type = ResourceType::Contents});
        ASSERT_TRUE(writer);
        ASSERT_TRUE(writer->add(10, {}));
        ASSERT_TRUE(writer->finalize());
        auto store = ResourceStore::open(directory);
        ASSERT_TRUE(store);
        auto item = store->get(10);
        ASSERT_TRUE(item) << item.error();
        EXPECT_TRUE(item->empty());
    }

    TEST_F(ResourceFormatTest, DuplicateIndexFilesAreRejected)
    {
        directStore();
        words("other.map", {0, 0});
        EXPECT_FALSE(ResourceStore::open(directory));
    }

    TEST_F(ResourceFormatTest, FontFragmentsReassembleAcrossBlobFiles)
    {
        auto writer = ResourceStoreWriter::createSequential(directory,
            {.type = ResourceType::Fonts, .maxFileSize = 1});
        ASSERT_TRUE(writer);
        const std::array<uint8_t, 4> signature{'O', 'T', 'T', 'O'};
        const std::array<uint8_t, 4> padding{};
        ASSERT_TRUE(writer->addData(signature));
        ASSERT_TRUE(writer->addData(padding));
        ASSERT_TRUE(writer->finalize());
        auto files = detail::resourcePaths(directory, ".rsc");
        ASSERT_TRUE(files);
        EXPECT_EQ(files->size(), 2);
        auto font = Font::open(directory);
        ASSERT_TRUE(font) << font.error();
        auto bytes = font->readAll();
        ASSERT_TRUE(bytes);
        EXPECT_EQ(bytes->size(), 8);
        auto type = font->detectType();
        ASSERT_TRUE(type);
        EXPECT_EQ(*type, "otf");
    }

    TEST_F(ResourceFormatTest, NamedWriterUsesItsDefaultCompressionLevel)
    {
        auto writer = NamedResourceStoreWriter::create(directory,
            {.type = ResourceType::Graphics, .compressionLevel = 9});
        ASSERT_TRUE(writer);
        ASSERT_TRUE(writer->add("a", std::vector<uint8_t>(1000, 'x')));
        ASSERT_TRUE(writer->finalize());
        auto index = NamedResourceStoreIndex::open(directory);
        ASSERT_TRUE(index);
        EXPECT_TRUE(index->itemAt(0)->second.isCompressed());
    }

    TEST_F(ResourceFormatTest, ExistingRscOutputsSkipInvalidPayloadsBeforeReading)
    {
        directStore();
        words("contents.map", {0, 2, 999, 0xFFFFFFFF, 999, 0xFFFFFFFF});
        auto store = ResourceStore::open(directory);
        ASSERT_TRUE(store);
        ASSERT_FALSE(store->itemAt(0));
        std::filesystem::create_directories(directory / "out" / "Contents");
        const std::array<uint8_t, 1> marker{'x'};
        write("out/Contents/000010.xml", marker);
        write("out/Contents/000020.xml", marker);
        ExportOptions options;
        options.outputDirectory = directory / "out";
        auto result = ResourceExporter::exportAll(*store, options, ResourceType::Contents);
        ASSERT_TRUE(result);
        EXPECT_EQ(result->skipped, 2);
        EXPECT_EQ(result->failed, 0);
    }

    TEST_F(ResourceFormatTest, ExistingNrscOutputSkipsInvalidCompressedPayloadBeforeReading)
    {
        auto writer = NamedResourceStoreWriter::create(directory, {.type = ResourceType::Graphics});
        ASSERT_TRUE(writer);
        ASSERT_TRUE(writer->add("image.png", std::vector<uint8_t>(1000, 'x'), 9));
        ASSERT_TRUE(writer->finalize());
        words("00000.nrsc", {0});
        auto store = NamedResourceStore::open(directory);
        ASSERT_TRUE(store);
        ASSERT_FALSE(store->get("image.png"));
        std::filesystem::create_directories(directory / "out" / "Graphics");
        const std::array<uint8_t, 1> marker{'x'};
        write("out/Graphics/image.png", marker);
        ExportOptions options;
        options.outputDirectory = directory / "out";
        auto result = ResourceExporter::exportAll(*store, options, ResourceType::Graphics);
        ASSERT_TRUE(result);
        EXPECT_EQ(result->skipped, 1);
        EXPECT_EQ(result->failed, 0);
    }

    TEST_F(ResourceFormatTest, UncompressedRscChunksRetainPayloadsAfterStoreDestruction)
    {
        words("contents.map", {0, 1, 0, 0});
        words("contents.idx", {1, 0, 42, 0});
        const std::array<uint8_t, 9> chunk{5, 0, 0, 0, 1, 0, 0, 0, 'x'};
        write("contents-0001.rsc", chunk);
        RetainedSpan payload;
        {
            auto store = ResourceStore::open(directory);
            ASSERT_TRUE(store);
            auto item = store->get(42);
            ASSERT_TRUE(item) << item.error();
            payload = std::move(*item);
        }
        EXPECT_EQ(*payload.utf8(), "x");
    }

    TEST_F(ResourceFormatTest, MappedIndexesSurviveMovesAndKeepLogicalPositions)
    {
        directStore();
        auto original = ResourceStoreIndex::open(directory);
        ASSERT_TRUE(original);
        auto moved = std::move(*original);
        EXPECT_EQ(*moved.indexOf(10), 0);
        EXPECT_EQ(moved.itemAt(0)->second.chunkGlobalOffset, 5);
        EXPECT_EQ(moved.findById(20)->chunkGlobalOffset, 0);
    }

    TEST_F(ResourceFormatTest, FontOpeningAndTypeDetectionDoNotReadTheRemainingFragments)
    {
        auto writer = ResourceStoreWriter::createSequential(directory, {.type = ResourceType::Fonts});
        ASSERT_TRUE(writer);
        const std::array<uint8_t, 4> signature{'O', 'T', 'T', 'O'};
        ASSERT_TRUE(writer->addData(signature));
        ASSERT_TRUE(writer->addData(signature));
        ASSERT_TRUE(writer->finalize());
        words("contents.map", {0, 2, 0, 0xFFFFFFFF, 999, 0xFFFFFFFF});
        auto font = Font::open(directory);
        ASSERT_TRUE(font) << font.error();
        auto type = font->detectType();
        ASSERT_TRUE(type);
        EXPECT_EQ(*type, "otf");
        EXPECT_FALSE(font->readAll());
        std::filesystem::create_directories(directory / "out/Fonts");
        const std::array<uint8_t, 1> marker{'x'};
        write("out/Fonts/" + font->name() + ".otf", marker);
        ExportOptions options;
        options.outputDirectory = directory / "out";
        auto result = ResourceExporter::exportFont(*font, options);
        ASSERT_TRUE(result);
        EXPECT_EQ(result->skipped, 1);
        EXPECT_EQ(result->failed, 0);
    }

    TEST_F(ResourceFormatTest, FontsStreamFragmentsAndReportOutputErrors)
    {
        auto writer = ResourceStoreWriter::createSequential(directory, {.type = ResourceType::Fonts});
        ASSERT_TRUE(writer);
        const std::array<uint8_t, 2> first{'O', 'T'};
        const std::array<uint8_t, 3> second{'T', 'O', 'x'};
        ASSERT_TRUE(writer->addData(first));
        ASSERT_TRUE(writer->addData(second));
        ASSERT_TRUE(writer->finalize());
        auto font = Font::open(directory);
        ASSERT_TRUE(font);
        auto type = font->detectType();
        ASSERT_TRUE(type);
        EXPECT_EQ(*type, "otf");
        std::ostringstream stream;
        auto written = font->writeTo(stream);
        ASSERT_TRUE(written);
        EXPECT_EQ(*written, 5);
        EXPECT_EQ(stream.str(), "OTTOx");
        stream.setstate(std::ios::badbit);
        EXPECT_FALSE(font->writeTo(stream));
    }

}
