//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "support/test_dictionary.hpp"
#include "binary/binary_view.hpp"
#include <array>

namespace MKD::test
{
    using ResourceTest = DictionaryTest;

    TEST_F(ResourceTest, ReadsSaberlionAndStreamsTheFont)
    {
        ASSERT_NE(birbs().graphics(), nullptr);
        const auto lion = require(birbs().graphics()->get("saberlion.png"));
        EXPECT_EQ(std::vector(lion.begin(), lion.end()), readFile(assets() / "saberlion.png"));
        EXPECT_FALSE(birbs().graphics()->get("missing-saberlion.png"));
        ASSERT_EQ(birbs().fonts().size(), 1);
        const auto& font = birbs().fonts().front();
        EXPECT_EQ(font.filename(), "BirbMincho.ttf");
        Workspace workspace;
        const auto output = workspace.path / font.filename();
        EXPECT_EQ(require(font.writeTo(output)), readFile(assets() / "BirbMincho.ttf").size());
        EXPECT_EQ(readFile(output), readFile(assets() / "BirbMincho.ttf"));
        EXPECT_FALSE(font.writeTo(output));
        EXPECT_FALSE(require(birbs().resource("audio/chirp.wav")).empty());
        EXPECT_FALSE(require(birbs().resource("birbs-ruby.css")).empty());
        EXPECT_FALSE(birbs().resource("../outside.txt"));
    }

    TEST(ResourceStores, RscRoundTripsSparseIdsEmptyPagesAndFileBoundaries)
    {
        Workspace workspace;
        for (const auto encrypted : {false, true})
        {
            const auto path = workspace.path / (encrypted ? "encrypted-birbs" : "plain-birbs");
            auto writer = require(ResourceStoreWriter::create(path, "test.birbs",
                                                              {
                                                                  .fileBytes = 64, .chunkBytes = 24,
                                                                  .encrypt = encrypted
                                                              }));
            for (uint32_t id = 1; id <= 9; ++id)
                require(writer.add(id * 2, bytes("birb number " + std::to_string(id))));
            require(writer.add(30, {}));
            require(writer.finish());
            auto store = require(ResourceStore::open(path, "test.birbs"));
            EXPECT_EQ(store.size(), 10);
            EXPECT_EQ(require(store.idAt(0)), 2);
            EXPECT_EQ(require(store.get(18)).utf8().value(), "birb number 9");
            EXPECT_TRUE(require(store.get(30)).empty());
            EXPECT_FALSE(store.get(3));
            EXPECT_FALSE(store.itemAt(store.size()));
            size_t blobs = 0;
            for (const auto& entry : std::filesystem::directory_iterator(path))
                blobs += entry.path().extension() == ".rsc";
            EXPECT_GT(blobs, 1);
        }
    }

    TEST(ResourceStores, NamedResourcesRetainRawAndCompressedBytesAfterClosing)
    {
        Workspace workspace;
        const auto path = workspace.path / "lions";
        auto writer = require(NamedResourceStoreWriter::create(path));
        require(writer.add("awake.txt", bytes("がおー")));
        require(writer.add("asleep.txt", bytes("すやすや"),
                           NamedResourceWriteOptions{.storage = NamedResourceStorage::Zlib}));
        require(writer.finish());
        const auto retained = [&]
        {
            auto store = require(NamedResourceStore::open(path));
            EXPECT_EQ(store.size(), 2);
            EXPECT_FALSE(store.get("invisible-lion"));
            return std::array{require(store.get("awake.txt")), require(store.get("asleep.txt"))};
        }();
        EXPECT_EQ(require(retained[0].utf8()), "がおー");
        EXPECT_EQ(require(retained[1].utf8()), "すやすや");
    }

    TEST(ResourceFormats, ReadsAnIndependentlySpecifiedRscPage)
    {
        Workspace workspace;
        const std::array<uint8_t, 16> map{0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255};
        const std::array<uint8_t, 16> index{1, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0};
        const std::array<uint8_t, 8> content{4, 0, 0, 0, 'g', 'a', 'o', 'o'};
        writeFile(workspace.path / "contents.map", map);
        writeFile(workspace.path / "contents.idx", index);
        writeFile(workspace.path / "contents-0001.rsc", content);
        auto store = require(ResourceStore::open(workspace.path));
        EXPECT_EQ(require(store.idAt(0)), 7);
        EXPECT_EQ(require(store.get(7)).utf8().value(), "gaoo");
    }

    TEST(ResourceFormats, ReadsExplicitEndianFieldsAndRejectsOutOfBoundsOffsets)
    {
        const std::array<uint8_t, 5> wire{0xff, 0x01, 0x02, 0x03, 0x04};
        const detail::BinaryView view(wire);
        EXPECT_EQ(require(view.readLittle<uint32_t>(1)), 0x04030201u);
        EXPECT_EQ(require(view.readBig<uint32_t>(1)), 0x01020304u);
        EXPECT_FALSE(view.readLittle<uint32_t>(2));
        EXPECT_FALSE(view.table(0, SIZE_MAX, 8));
        EXPECT_FALSE(view.slice(SIZE_MAX, 1));
    }

    TEST(ResourceFormats, RejectsTruncatedIndexesRatherThanReadingPastTheFile)
    {
        Workspace workspace;
        const auto path = workspace.path / "broken";
        writeFile(path, bytes("birb"));
        EXPECT_FALSE(Keystore::open(path));
        EXPECT_FALSE(HeadlineStore::open(path));
        EXPECT_FALSE(AppendixEntryList::open(path));
        EXPECT_FALSE(Font::open(path));
        const auto contents = workspace.path / "contents";
        std::filesystem::create_directory(contents);
        writeFile(contents / "contents.map", bytes("bad"));
        writeFile(contents / "contents.idx", bytes("bad"));
        EXPECT_FALSE(ResourceStore::open(contents));
    }

    TEST(ResourceFormats, KeystoreVersionsKeepExactOnlyKeysAndInverseLookup)
    {
        Workspace workspace;
        for (const auto version : {KeystoreVersion::Version1, KeystoreVersion::Version2})
        {
            const auto path = workspace.path / (version == KeystoreVersion::Version1 ? "one.keystore" : "two.keystore");
            auto writer = require(KeystoreWriter::create(path, {.version = version}));
            require(writer.add("birb", {7, 0}));
            require(writer.add("birb song", {7, 0x4001}, {.exactOnly = true}));
            require(writer.finish());
            auto store = require(Keystore::open(path));
            const auto exact = require(keystoreSearchResults(store, "birb song", SearchMode::Exact));
            EXPECT_EQ(require(store.keysForEntry({7, 0x4001})), (std::vector<std::string_view>{"birb song"}));
            ASSERT_EQ(exact.size(), 1);
            EXPECT_EQ(exact.front().entryIds, (std::vector<EntryId>{{7, 0x4001}}));
            const auto prefix = require(keystoreSearchResults(store, "birb", SearchMode::Prefix));
            ASSERT_EQ(prefix.size(), 1);
            EXPECT_EQ(prefix.front().entryIds, (std::vector<EntryId>{{7, 0}}));
        }
    }

    TEST_F(ResourceTest, BrokenOrderTablesDoNotSilentlyUseNumericOrder)
    {
        Workspace workspace;
        const auto path = workspace.path / "Birbs";
        std::filesystem::copy(dictionaries() / "Birbs", path, std::filesystem::copy_options::recursive);
        const auto order = path / "Contents/birbs/contents/contents-order.dat";
        auto data = readFile(order);
        data[8] = 99;
        writeFile(order, data);
        EXPECT_FALSE(DictionaryProduct::openAtPath(path));
        writeFile(order, {});
        EXPECT_FALSE(DictionaryProduct::openAtPath(path));
    }
}
