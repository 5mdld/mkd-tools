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
#include "resource/rsc/resource_store_crypto.hpp"
#include "MKD/resource/resource_store_writer.hpp"
#include "resource/nrsc/named_resource_store_index.hpp"
#include "MKD/resource/named_resource_store_writer.hpp"
#include "resource/keystore/entry_id_codec.hpp"
#include "MKD/resource/keystore_writer.hpp"
#include "resource/detail/fragment_writer.hpp"

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
        std::filesystem::remove(directory);
        auto writer = detail::FragmentWriter::create(directory, ResourceType::Contents);
        ASSERT_TRUE(writer);
        ASSERT_TRUE(writer->add(a));
        ASSERT_TRUE(writer->add({}));
        ASSERT_TRUE(writer->finish());
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
        std::filesystem::remove(directory);
        auto writer = ResourceStoreWriter::create(directory, "fixture",
            {.fileBytes = 1, .chunkBytes = 1, .encrypt = true});
        ASSERT_TRUE(writer);
        ASSERT_TRUE(writer->add(20, std::array<uint8_t, 1>{'a'}));
        ASSERT_TRUE(writer->add(10, std::array<uint8_t, 1>{'b'}));
        ASSERT_TRUE(writer->finish());
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
        std::filesystem::remove(directory);
        auto writer = NamedResourceStoreWriter::create(directory);
        ASSERT_TRUE(writer);
        ASSERT_TRUE(writer->add("plain", std::array<uint8_t, 1>{'a'}));
        ASSERT_TRUE(writer->add("compressed", std::vector<uint8_t>(1000, 'b'),
                                NamedResourceWriteOptions{.storage = NamedResourceStorage::Zlib, .compressionLevel = 9}));
        ASSERT_TRUE(writer->finish());
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

    TEST_F(ResourceFormatTest, KeystoreCodecHandlesMixedEndianFieldsAndTruncation)
    {
        const std::array<uint8_t, 9> bytes{1, 0, 0x24, 0x12, 0x34, 0x56, 0x78, 0x9a, 0};
        auto ids = detail::keystore::decodeEntryIds(bytes);
        ASSERT_TRUE(ids) << ids.error();
        ASSERT_EQ(ids->size(), 1);
        EXPECT_EQ(ids->front(), (EntryId{0x123456, 0x789a}));
        for (size_t size = 0; size < 8; ++size)
            EXPECT_FALSE(detail::keystore::decodeEntryIds(std::span(bytes).first(size)));
        const std::array<uint8_t, 4> hugeCount{0xff, 0xff, 0xff, 0xff};
        EXPECT_FALSE(detail::keystore::decodeEntryIds(hugeCount, true));
    }

    TEST_F(ResourceFormatTest, KeystoreRoundTripAndInverseLookup)
    {
        auto writer = KeystoreWriter::create(directory / "headword.keystore");
        ASSERT_TRUE(writer);
        ASSERT_TRUE(writer->add("key", EntryId{0x123456, 0x789a}));
        ASSERT_TRUE(writer->finish());
        auto store = Keystore::open(directory / "headword.keystore");
        ASSERT_TRUE(store) << store.error();
        auto ids = store->entryIdsAt(KeystoreIndex::Prefix, 0);
        ASSERT_TRUE(ids);
        EXPECT_EQ(ids->front(), (EntryId{0x123456, 0x789a}));
        auto keys = store->keysForEntry(ids->front());
        ASSERT_TRUE(keys);
        EXPECT_EQ(*keys, std::vector<std::string_view>{"key"});
        for (auto entry : store->entries(KeystoreIndex::Prefix))
        {
            ASSERT_TRUE(entry);
            EXPECT_EQ(entry->key, "key");
        }
    }

    TEST_F(ResourceFormatTest, InvalidKeystoreOffsetsReachTheCallerAsErrors)
    {
        words("bad.keystore", {0x10000, 0, 16, 20, 0, 4, 0, 20, 0, 0, 1, 0xFFFFFFFF});
        auto store = Keystore::open(directory / "bad.keystore");
        ASSERT_TRUE(store) << store.error();
        EXPECT_FALSE(store->entryAt(KeystoreIndex::Prefix, 0));
        for (auto entry : store->entries(KeystoreIndex::Prefix))
            EXPECT_FALSE(entry);
        EXPECT_FALSE(keystoreSearch(*store, "a", SearchMode::Exact));
    }

    TEST_F(ResourceFormatTest, HeadlinesDecodeUnalignedUtf16AndPreserveFallback)
    {
        detail::BinaryBuffer bytes;
        for (uint32_t value : {0, 0, 1, 24, 49, 24, 7}) bytes.writeLittle(value);
        bytes.writeLittle(uint16_t{0});
        bytes.writeLittle(uint16_t{0});
        for (uint32_t value : {0, 0, 0, 4}) bytes.writeLittle(value);
        bytes.writeByte(0);
        for (uint16_t value : {u'a', u'\0', u'b', u'\0'}) bytes.writeLittle(value);
        write("headline.headlinestore", bytes.data());
        auto store = HeadlineStore::open(directory / "headline.headlinestore");
        ASSERT_TRUE(store) << store.error();
        auto entry = store->entryAt(0);
        ASSERT_TRUE(entry);
        EXPECT_EQ(entry->headline, u"a");
        EXPECT_EQ(entry->sortingHeadline, u"b");
        EXPECT_TRUE(store->componentsForEntryId({7, 3}));
        EXPECT_FALSE(store->componentsForEntryId({7, 3}, false));
        auto moved = std::move(*store);
        EXPECT_EQ(entry->headline, u"a");
    }

    TEST_F(ResourceFormatTest, LoaderDistinguishesAbsenceFromCorruption)
    {
        std::filesystem::create_directories(directory / "Contents" / "DICT");
        auto paths = DictionaryPaths::create(directory);
        ASSERT_TRUE(paths);
        ResourceLoader loader(*paths);
        auto absent = loader.loadEntries("DICT", "id");
        ASSERT_TRUE(absent);
        EXPECT_FALSE(*absent);
        std::filesystem::create_directories(directory / "Contents" / "DICT" / "contents");
        EXPECT_FALSE(loader.loadEntries("DICT", "id"));
        std::filesystem::create_directories(directory / "Contents" / "DICT" / "graphics");
        EXPECT_FALSE(loader.loadGraphics("DICT"));
        std::filesystem::create_directories(directory / "Contents" / "DICT" / "audio");
        words("Contents/DICT/audio/index.nidx", {0, 100});
        EXPECT_FALSE(loader.loadAudio("DICT", "id"));
        std::filesystem::create_directories(directory / "Contents" / "DICT" / "key");
        words("Contents/DICT/key/bad.keystore", {0});
        EXPECT_FALSE(loader.loadKeystores("DICT"));
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
        std::filesystem::remove(directory);
        auto writer = ResourceStoreWriter::create(directory);
        ASSERT_TRUE(writer);
        ASSERT_TRUE(writer->add(10, {}));
        ASSERT_TRUE(writer->finish());
        auto store = ResourceStore::open(directory);
        ASSERT_TRUE(store);
        auto item = store->get(10);
        ASSERT_TRUE(item) << item.error();
        EXPECT_TRUE(item->empty());
    }

    TEST_F(ResourceFormatTest, VersionTwoKeystoresApplyTheEmbeddedConversionTable)
    {
        detail::BinaryBuffer bytes;
        for (uint32_t value : {0x20000, 0, 32, 42, 70, 0, 0, 0, 7}) bytes.writeLittle(value);
        bytes.writeByte(0);
        bytes.writeByte('k');
        bytes.writeByte(0);
        bytes.writeLittle(uint16_t{1});
        bytes.writeByte(0);
        for (uint32_t value : {4, 0, 20, 0, 0, 1, 0, 1, 42}) bytes.writeLittle(value);
        bytes.writeLittle(uint16_t{7});
        bytes.writeLittle(uint16_t{0});
        write("converted.keystore", bytes.data());
        auto store = Keystore::open(directory / "converted.keystore");
        ASSERT_TRUE(store) << store.error();
        auto ids = store->entryIdsAt(KeystoreIndex::Prefix, 0);
        ASSERT_TRUE(ids);
        EXPECT_EQ(*ids, (std::vector<EntryId>{{42, 7}}));
        auto keys = store->keysForEntry({42, 7});
        ASSERT_TRUE(keys);
        EXPECT_EQ(*keys, std::vector<std::string_view>{"k"});
    }

    TEST_F(ResourceFormatTest, AppendixTablesUseCheckedOffsetsAndBorrowedLabels)
    {
        detail::BinaryBuffer bytes;
        for (uint32_t value : {1, 1, 1, 0, 0, 0, 0, 44, 48, 7}) bytes.writeLittle(value);
        bytes.writeLittle(uint16_t{2});
        bytes.writeLittle(uint16_t{0});
        bytes.writeLittle(uint32_t{52});
        bytes.writeLittle(uint32_t{u'a'});
        bytes.writeByte('x');
        bytes.writeByte(0);
        write("list.entries", bytes.data());
        auto list = AppendixEntryList::open(directory / "list.entries");
        ASSERT_TRUE(list) << list.error();
        auto entry = list->entryAt(0);
        ASSERT_TRUE(entry);
        EXPECT_EQ(entry->entryId, (EntryId{7, 2}));
        EXPECT_EQ(*entry->label, "x");
        EXPECT_EQ(*entry->firstCharacter, u'a');
        words("bad.entries", {0xFFFFFFFF, 1, 1, 0, 0, 0, 0, 0, 0});
        EXPECT_FALSE(AppendixEntryList::open(directory / "bad.entries"));
    }

    TEST_F(ResourceFormatTest, MissingTerminatorsAndOverlappingHeadlineSectionsAreRejected)
    {
        words("bad.headlinestore", {0, 0, 1, 24, 44, 20, 1, 0, 0, 0, 0, 0x00610061});
        EXPECT_FALSE(HeadlineStore::open(directory / "bad.headlinestore"));
        words("bad.headlinestore", {0, 0, 1, 24, 40, 20, 1, 0, 0, 0, 0, 0});
        EXPECT_FALSE(HeadlineStore::open(directory / "bad.headlinestore"));
        words("bad.headlinestore", {0, 0, 0xFFFFFFFF, 24, 24, 24});
        EXPECT_FALSE(HeadlineStore::open(directory / "bad.headlinestore"));
    }

    TEST_F(ResourceFormatTest, DuplicateIndexFilesAreRejected)
    {
        directStore();
        words("other.map", {0, 0});
        EXPECT_FALSE(ResourceStore::open(directory));
    }

    TEST_F(ResourceFormatTest, ProductOpeningPropagatesMalformedResourceErrors)
    {
        const auto fixture = std::filesystem::path(__FILE__).parent_path().parent_path() / "test-data" / "TESTDICT";
        const auto product = directory / "TESTDICT";
        std::filesystem::copy(fixture, product, std::filesystem::copy_options::recursive);
        const auto index = product / "Contents" / "TESTDICT" / "graphics" / "index.nidx";
        { std::ofstream file(index, std::ios::binary | std::ios::trunc); file.put(0); }
        auto opened = DictionaryProduct::openAtPath(product);
        ASSERT_FALSE(opened);
        EXPECT_NE(opened.error().find("Graphics"), std::string::npos);
        EXPECT_NE(opened.error().find("TESTDICT"), std::string::npos);
    }

    TEST_F(ResourceFormatTest, ArbitrarySmallInputsFailWithoutUncheckedMemoryAccess)
    {
        std::mt19937 generator(42);
        for (size_t i = 0; i < 128; ++i)
        {
            std::vector<uint8_t> bytes(i);
            for (auto& byte : bytes)
                byte = static_cast<uint8_t>(generator());
            write("fuzz.keystore", bytes);
            write("fuzz.headlinestore", bytes);
            write("fuzz.entries", bytes);
            write("index.nidx", bytes);
            write("contents.map", bytes);
            EXPECT_NO_THROW((void)Keystore::open(directory / "fuzz.keystore"));
            EXPECT_NO_THROW((void)HeadlineStore::open(directory / "fuzz.headlinestore"));
            EXPECT_NO_THROW((void)AppendixEntryList::open(directory / "fuzz.entries"));
            EXPECT_NO_THROW((void)NamedResourceStoreIndex::open(directory));
            EXPECT_NO_THROW((void)ResourceStoreIndex::open(directory));
            EXPECT_NO_THROW((void)detail::keystore::decodeEntryIds(bytes));
            EXPECT_NO_THROW((void)ResourceStoreCrypto::decrypt(bytes, {}));
        }
    }

    TEST_F(ResourceFormatTest, FontFragmentsReassembleAcrossBlobFiles)
    {
        std::filesystem::remove(directory);
        auto writer = detail::FragmentWriter::create(directory, ResourceType::Fonts, 1);
        ASSERT_TRUE(writer);
        const std::array<uint8_t, 4> signature{'O', 'T', 'T', 'O'};
        const std::array<uint8_t, 4> padding{};
        ASSERT_TRUE(writer->add(signature));
        ASSERT_TRUE(writer->add(padding));
        ASSERT_TRUE(writer->finish());
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
        std::filesystem::remove(directory);
        auto writer = NamedResourceStoreWriter::create(directory,
            {.resources = {.storage = NamedResourceStorage::Zlib, .compressionLevel = 9}});
        ASSERT_TRUE(writer);
        ASSERT_TRUE(writer->add("a", std::vector<uint8_t>(1000, 'x')));
        ASSERT_TRUE(writer->finish());
        auto index = NamedResourceStoreIndex::open(directory);
        ASSERT_TRUE(index);
        EXPECT_TRUE(index->itemAt(0)->second.isCompressed());
    }

    TEST_F(ResourceFormatTest, EntryIdsWorkAsStandardUnorderedKeys)
    {
        std::unordered_set<EntryId> ids;
        ids.insert({42, 7});
        ids.insert({42, 7});
        ids.insert({42, 8});
        ids.insert({43, 7});
        EXPECT_EQ(ids.size(), 3);
        EXPECT_TRUE(ids.contains({42, 7}));
        EXPECT_FALSE(ids.contains({43, 8}));
    }

    TEST_F(ResourceFormatTest, FullHeadlineComposesUtf16Components)
    {
        const HeadlineComponents components{
            .prefix = u"〈",
            .headline = u"辞書",
            .suffix = u"〉",
            .sortingHeadline = u"じしょ",
            .entryId = {42, 7},
        };
        EXPECT_EQ(components.full(), u"〈辞書〉");
        EXPECT_TRUE(HeadlineComponents{}.full().empty());
    }

    TEST_F(ResourceFormatTest, HeadlineExportAccountsForTheExactUtf8BytesWritten)
    {
        const auto fixture = std::filesystem::path(__FILE__).parent_path().parent_path() /
            "test-data" / "TESTDICT" / "Contents" / "TESTDICT" / "headline" / "headline.headlinestore";
        auto store = HeadlineStore::open(fixture);
        ASSERT_TRUE(store) << store.error();
        ExportOptions options;
        options.createSubdirectories = false;
        DictionaryResources resources;
        resources.headlines.push_back(std::move(*store));
        Dictionary dictionary({}, std::move(resources));
        auto result = exportToDirectory(dictionary, directory, options);
        ASSERT_TRUE(result) << result.error();
        EXPECT_EQ(result->exported, 1);
        EXPECT_EQ(result->totalBytes, std::filesystem::file_size(directory / (std::string(dictionary.headlines().front().filename()) + ".tsv")));
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
        DictionaryResources resources;
        resources.entries = std::move(*store);
        Dictionary dictionary({}, std::move(resources));
        auto result = exportToDirectory(dictionary, directory / "out", options);
        ASSERT_TRUE(result);
        EXPECT_EQ(result->skipped, 2);
        EXPECT_EQ(result->failed, 0);
    }

    TEST_F(ResourceFormatTest, ExistingNrscOutputSkipsInvalidCompressedPayloadBeforeReading)
    {
        std::filesystem::remove(directory);
        auto writer = NamedResourceStoreWriter::create(directory);
        ASSERT_TRUE(writer);
        ASSERT_TRUE(writer->add("image.png", std::vector<uint8_t>(1000, 'x'),
                                NamedResourceWriteOptions{.storage = NamedResourceStorage::Zlib, .compressionLevel = 9}));
        ASSERT_TRUE(writer->finish());
        words("00000.nrsc", {0});
        auto store = NamedResourceStore::open(directory);
        ASSERT_TRUE(store);
        ASSERT_FALSE(store->get("image.png"));
        std::filesystem::create_directories(directory / "out" / "Graphics");
        const std::array<uint8_t, 1> marker{'x'};
        write("out/Graphics/image.png", marker);
        ExportOptions options;
        DictionaryResources resources;
        resources.graphics = std::move(*store);
        Dictionary dictionary({}, std::move(resources));
        auto result = exportToDirectory(dictionary, directory / "out", options);
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

    TEST_F(ResourceFormatTest, MappedHeadlinesReturnOwnedComponents)
    {
        const auto fixture = std::filesystem::path(__FILE__).parent_path().parent_path() /
            "test-data/TESTDICT/Contents/TESTDICT/headline/headline.headlinestore";
        HeadlineComponents components;
        std::u16string expected;
        {
            auto store = HeadlineStore::open(fixture);
            ASSERT_TRUE(store);
            auto entry = store->entryAt(0);
            ASSERT_TRUE(entry);
            expected = entry->full();
            components = std::move(*entry);
        }
        EXPECT_EQ(components.full(), expected);
        EXPECT_FALSE(expected.empty());
    }

    TEST_F(ResourceFormatTest, FontOpeningAndTypeDetectionDoNotReadTheRemainingFragments)
    {
        std::filesystem::remove(directory);
        auto writer = detail::FragmentWriter::create(directory);
        ASSERT_TRUE(writer);
        const std::array<uint8_t, 4> signature{'O', 'T', 'T', 'O'};
        ASSERT_TRUE(writer->add(signature));
        ASSERT_TRUE(writer->add(signature));
        ASSERT_TRUE(writer->finish());
        words("font.map", {0, 2, 0, 0xFFFFFFFF, 999, 0xFFFFFFFF});
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
        DictionaryResources resources;
        resources.fonts.push_back(std::move(*font));
        Dictionary dictionary({}, std::move(resources));
        auto result = exportToDirectory(dictionary, directory / "out", options);
        ASSERT_TRUE(result);
        EXPECT_EQ(result->skipped, 1);
        EXPECT_EQ(result->failed, 0);
    }

    TEST_F(ResourceFormatTest, FontsStreamFragmentsAndReportOutputErrors)
    {
        std::filesystem::remove(directory);
        auto writer = detail::FragmentWriter::create(directory);
        ASSERT_TRUE(writer);
        const std::array<uint8_t, 2> first{'O', 'T'};
        const std::array<uint8_t, 3> second{'T', 'O', 'x'};
        ASSERT_TRUE(writer->add(first));
        ASSERT_TRUE(writer->add(second));
        ASSERT_TRUE(writer->finish());
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

    TEST_F(ResourceFormatTest, ProductMetadataLoadingDoesNotOpenResources)
    {
        const auto fixture = std::filesystem::path(__FILE__).parent_path().parent_path() / "test-data/TESTDICT";
        const auto product = directory / "TESTDICT";
        std::filesystem::copy(fixture, product, std::filesystem::copy_options::recursive);
        words("TESTDICT/Contents/TESTDICT/graphics/index.nidx", {0});
        auto metadata = DictionaryProduct::loadMetadata(product);
        ASSERT_TRUE(metadata) << metadata.error();
        ASSERT_EQ(metadata->dictionaries().size(), 1);
        EXPECT_EQ(metadata->dictionaries()[0].id(), "Test.dict");
        EXPECT_FALSE(metadata->dictionaries()[0].hasEntries());
        EXPECT_FALSE(DictionaryProduct::openAtPath(product));
    }

    TEST_F(ResourceFormatTest, ProductOpeningLoadsAndValidatesAllContents)
    {
        const auto fixture = std::filesystem::path(__FILE__).parent_path().parent_path() / "test-data/TESTDICT";
        const auto product = directory / "TESTDICT";
        std::filesystem::copy(fixture, product, std::filesystem::copy_options::recursive);
        const auto metadata = product / "Contents/TESTDICT.json";
        std::ofstream json(metadata);
        json << R"({"DSProductContents":[
            {"DSContentDirectory":"TESTDICT","DSContentIdentifier":"Test.dict"},
            {"DSContentDirectory":"SECOND","DSContentIdentifier":"Second.dict"}]})";
        json.close();
        std::filesystem::copy(product / "Contents/TESTDICT", product / "Contents/SECOND",
                              std::filesystem::copy_options::recursive);
        auto opened = DictionaryProduct::openAtPath(product);
        ASSERT_TRUE(opened) << opened.error();
        ASSERT_EQ(opened->dictionaries().size(), 2);
        EXPECT_EQ(opened->dictionaries()[0].id(), "Test.dict");
        EXPECT_EQ(opened->dictionaries()[1].id(), "Second.dict");
        EXPECT_TRUE(opened->dictionaries()[0].hasEntries());
        EXPECT_TRUE(opened->dictionaries()[1].hasEntries());
        words("TESTDICT/Contents/SECOND/graphics/index.nidx", {0});
        EXPECT_FALSE(DictionaryProduct::openAtPath(product));
    }


    TEST_F(ResourceFormatTest, DirectorySourcesReportMissingAndNonDirectoryRoots)
    {
        DirectoryDictionarySource missing(directory / "missing");
        auto listed = missing.findAllAvailable();
        ASSERT_FALSE(listed);
        EXPECT_NE(listed.error().find("does not exist"), std::string::npos);
        EXPECT_FALSE(missing.findById("TESTDICT"));

        words("file", {0});
        DirectoryDictionarySource file(directory / "file");
        auto result = file.findAllAvailable();
        ASSERT_FALSE(result);
        EXPECT_NE(result.error().find("Not a directory"), std::string::npos);
        EXPECT_FALSE(file.findById("TESTDICT"));
    }

    TEST_F(ResourceFormatTest, DirectorySourcesFilterInvalidProductsAndResolveIds)
    {
        const auto fixture = std::filesystem::path(__FILE__).parent_path().parent_path() / "test-data/TESTDICT";
        std::filesystem::copy(fixture, directory / "TESTDICT", std::filesystem::copy_options::recursive);
        std::filesystem::create_directories(directory / "unrelated/Contents");
        words("plain-file", {0});
        DirectoryDictionarySource source(directory);
        auto dictionaries = source.findAllAvailable();
        ASSERT_TRUE(dictionaries) << dictionaries.error();
        ASSERT_EQ(dictionaries->size(), 1);
        EXPECT_EQ(dictionaries->front().id, "TESTDICT");
        auto found = source.findById("TESTDICT");
        ASSERT_TRUE(found);
        EXPECT_EQ(found->path, directory / "TESTDICT");
        EXPECT_FALSE(source.findById("unrelated"));
        EXPECT_FALSE(source.findById("missing"));
    }

    TEST_F(ResourceFormatTest, DirectorySourcesPropagateMetadataInspectionErrors)
    {
        std::filesystem::create_directories(directory / "broken/Contents");
        words("broken/ContentInfo.plist", {0});
        std::error_code error;
        std::filesystem::create_symlink("broken.json", directory / "broken/Contents/broken.json", error);
        if (error) GTEST_SKIP() << "Cannot create a symlink: " << error.message();
        DirectoryDictionarySource source(directory);
        auto listed = source.findAllAvailable();
        ASSERT_FALSE(listed);
        EXPECT_NE(listed.error().find("Cannot inspect"), std::string::npos);
        EXPECT_FALSE(source.findById("broken"));
    }

}
