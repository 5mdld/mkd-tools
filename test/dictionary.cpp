//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "support/test_dictionary.hpp"
#include <algorithm>

namespace MKD::test
{
    TEST_F(DictionaryTest, OpensTheFlockAndDiscoversBothProducts)
    {
        EXPECT_EQ(product.metadata().version, "1.0");
        EXPECT_EQ(birbs().id(), "test.birbs");
        EXPECT_EQ(birbs().resourceCount(ResourceType::Contents), 3);
        EXPECT_NE(require(birbs().entryUtf8ById(3)).find("がおー"), std::string::npos);
        EXPECT_EQ(require(birbs().headlineForEntryId({1, 0})), "鶫");
        EXPECT_EQ(require(birbs().sortingHeadlineForEntryId({1, 0})), "つぐみ");
        const auto found = require(DirectoryDictionarySource(dictionaries()).findAllAvailable());
        EXPECT_EQ(found.size(), 2);
        EXPECT_EQ(require(DirectoryDictionarySource(dictionaries()).findById("BirbKanji")).id, "BirbKanji");
        EXPECT_TRUE(product.iconPath().has_value());
    }

    TEST_F(DictionaryTest, BrowsesInAuthoredOrderWithoutWrapping)
    {
        const auto& order = birbs().entryOrder();
        EXPECT_EQ(std::ranges::to<std::vector>(order.pages()), (std::vector<uint32_t>{3, 1, 2}));
        EXPECT_EQ(require(order.previous(3)), std::nullopt);
        EXPECT_EQ(require(order.next(2)), std::nullopt);
        EXPECT_EQ(require(order.next(3)), 1);
        EXPECT_EQ(require(order.previous(2)), 1);
        EXPECT_FALSE(order.indexOf(99));
        EXPECT_FALSE(order.pageAt(order.size()));
    }

    TEST_F(DictionaryTest, ReadsAppendixTargetsAndAlternativeHeadlines)
    {
        const auto& lists = birbs().appendixEntryLists();
        ASSERT_EQ(lists.size(), 1);
        EXPECT_EQ(lists.front().title(), "小鳥と獅子");
        const auto entry = require(lists.front().entryAt(1));
        EXPECT_EQ(entry.entryId, (EntryId{3, 0}));
        EXPECT_EQ(entry.label, "セイバーライオン");
        EXPECT_EQ(birbs().headlines().size(), 3);
        std::vector<std::u16string> labels;
        for (const auto& store : birbs().headlines())
            labels.push_back(require(store.componentsForEntryId({1, 0}, false)).headline);
        EXPECT_NE(std::ranges::find(labels, u"つぐみ"), labels.end());
        EXPECT_NE(require(birbs().resource("guide.html")).utf8().value().find("hungry lion"), std::string::npos);
    }

    TEST(DictionaryReading, KeepsRetainedPagesAfterTheDictionaryCloses)
    {
        const auto page = []
        {
            auto product = require(DictionaryProduct::openAtPath(dictionaries() / "BirbKanji"));
            return require(product.dictionaries().front().entryById(1)).data;
        }();
        EXPECT_NE(require(page.utf8()).find("鳥語"), std::string::npos);
        const auto slice = require(page.slice(0, 5));
        EXPECT_EQ(require(slice.utf8()), "<html");
        EXPECT_FALSE(page.slice(page.size(), 1));
    }

    TEST(DictionaryReading, LoadsMetadataWithoutReadingDamagedResources)
    {
        Workspace workspace;
        const auto path = workspace.path / "Birbs";
        std::filesystem::copy(dictionaries() / "Birbs", path, std::filesystem::copy_options::recursive);
        writeFile(path / "Contents/birbs/contents/contents.idx", bytes("broken index"));
        EXPECT_TRUE(DictionaryProduct::loadMetadata(path));
        EXPECT_FALSE(DictionaryProduct::openAtPath(path));
        EXPECT_FALSE(DictionaryProduct::openAtPath(workspace.path / "MissingBirb"));
        EXPECT_FALSE(DirectoryDictionarySource(workspace.path / "MissingBirb").findAllAvailable());
    }

    TEST_F(DictionaryTest, NumericOrderIsTheNativeFallback)
    {
        Workspace workspace;
        const auto path = workspace.path / "Birbs";
        std::filesystem::copy(dictionaries() / "Birbs", path, std::filesystem::copy_options::recursive);
        std::filesystem::remove(path / "Contents/birbs/contents/contents-order.dat");
        auto reopened = require(DictionaryProduct::openAtPath(path));
        const auto& order = reopened.dictionaries().front().entryOrder();
        EXPECT_EQ(std::ranges::to<std::vector>(order.pages()), (std::vector<uint32_t>{1, 2, 3}));
        EXPECT_EQ(require(order.previous(1)), std::nullopt);
        EXPECT_EQ(require(order.next(3)), std::nullopt);
    }
}
