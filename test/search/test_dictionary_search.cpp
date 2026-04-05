//
// kiwakiwaaにより 2026/03/13 に作成されました。
//

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>
#include <iomanip>

#include "../test_listener.hpp"
#include "MKD/dictionary/dictionary_product.hpp"
#include "MKD/platform/macos/macos_dictionary_source.hpp"
#include "MKD/dictionary/dictionary_search.hpp"
#include "platform/macos/fs.hpp"

using namespace MKD;

class DictionarySearchTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto containerPath =
            MKD::macOS::getContainerPathByGroupIdentifier(MKD::MONOKAKIDO_GROUP_ID);

        dictionariesPath_ = containerPath / MKD::DICTIONARIES_SUBPATH;
    }

    [[nodiscard]] Dictionary loadDictionary(const std::string& id) const
    {
        const auto dictPath = dictionariesPath_ / id;
        auto product = DictionaryProduct::openAtPath(dictPath);

        auto& dicts = product->dictionaries();

        return std::move(dicts.front());
    }

    static void printInfo(const std::vector<std::string>& keys, const std::vector<EntryId>& entryIds)
    {
        for (auto& key : keys)
        {
            test::verbosePrint("{}, ", key);
        }
        test::verbosePrint("\n");
        for (auto& [pageId, itemId] : entryIds)
        {
            test::verbosePrint("{:06}-{:04x}\n", pageId, itemId);
        }
    }

    // Helper function to measure and print search time
    template<typename Func>
    auto measureSearchTime(const std::string& searchName, Func&& searchFunc)
    {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = searchFunc();
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double milliseconds = static_cast<double>(duration.count()) / 1000.0;

        test::verbosePrint("⏱️  {} search took: {:.3f} ms ({} μs)\n",
                          searchName, milliseconds, duration.count());

        return result;
    }

    std::filesystem::path dictionariesPath_;
};

TEST_F(DictionarySearchTest, KANKENKJ2_KanjiHeadwordSearch)
{
    auto dict = loadDictionary("KANKENKJ2");
    DictionarySearch search(dict);

    auto result = measureSearchTime("KANKENKJ2 headword", [&]() {
        return search.search("愛");
    });

    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->empty());
    printInfo(result->matchedKeys, result->entries);
}

TEST_F(DictionarySearchTest, KANKENKJ2_CompoundKanjiSearch)
{
    auto dict = loadDictionary("KANKENKJ2");
    DictionarySearch search(dict);

    auto result = measureSearchTime("KANKENKJ2 compound", [&]() {
        return search.search("愛情");
    });

    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->empty());

    ASSERT_FALSE(result->matchedKeys.empty());
    printInfo(result->matchedKeys, result->entries);
}

TEST_F(DictionarySearchTest, KANKENKJ2_PrefixSearch)
{
    auto dict = loadDictionary("KANKENKJ2");
    DictionarySearch search(dict);

    SearchOptions options;
    options.type = SearchMode::Prefix;

    auto result = measureSearchTime("KANKENKJ2 prefix", [&]() {
        return search.search("愛", options);
    });

    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->empty());
    printInfo(result->matchedKeys, result->entries);
}

TEST_F(DictionarySearchTest, OKO12_HeadwordSearch)
{
    auto dict = loadDictionary("KOGO3");
    DictionarySearch search(dict);

    auto result = measureSearchTime("KOGO3 headword", [&]() {
        return search.search("学生");
    });

    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->empty());
    printInfo(result->matchedKeys, result->entries);
}

TEST_F(DictionarySearchTest, OKO12_IdiomScopeFallback)
{
    auto dict = loadDictionary("KOGO3");
    DictionarySearch search(dict);

    SearchOptions options;
    options.scope = SearchScope::Headword;

    auto result = measureSearchTime("KOGO3 idiom fallback", [&]() {
        return search.search("気に掛かる", options);
    });

    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->empty());

    // fallback flag expected
    EXPECT_TRUE(result->flags & 4);
    printInfo(result->matchedKeys, result->entries);
}

TEST_F(DictionarySearchTest, OKO12_JoinedQueryFallback)
{
    auto dict = loadDictionary("KOGO3");
    DictionarySearch search(dict);

    auto result = measureSearchTime("KOGO3 joined query", [&]() {
        return search.search("取り 扱い");
    });

    ASSERT_TRUE(result.has_value());
    printInfo(result->matchedKeys, result->entries);
}

TEST_F(DictionarySearchTest, KOGO3_HeadwordSearch)
{
    auto dict = loadDictionary("SKOGO");
    DictionarySearch search(dict);

    auto result = measureSearchTime("KOGO3 headword", [&]() {
        return search.search("あはれ");
    });

    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->empty());
    printInfo(result->matchedKeys, result->entries);
}

TEST_F(DictionarySearchTest, SearchCancellation)
{
    auto dict = loadDictionary("KOGO3");
    DictionarySearch search(dict);

    search.cancel();

    auto result = measureSearchTime("cancelled search", [&]() {
        return search.search("日本");
    });

    ASSERT_FALSE(result.has_value());
}

TEST_F(DictionarySearchTest, DictionaryEntryAccessHelpers)
{
    auto dict = loadDictionary("KOGO3");
    ASSERT_TRUE(dict.hasEntries());

    auto firstEntry = dict.entryByIndex(0);
    ASSERT_TRUE(firstEntry.has_value());
    EXPECT_FALSE(firstEntry->data.empty());

    auto utf8View = firstEntry->asUtf8StringView();
    ASSERT_TRUE(utf8View.has_value());
    EXPECT_FALSE(utf8View->empty());

    auto sameEntry = dict.entryById(firstEntry->itemId);
    ASSERT_TRUE(sameEntry.has_value());

    auto utf8ById = dict.entryUtf8ById(firstEntry->itemId);
    ASSERT_TRUE(utf8ById.has_value());
    EXPECT_FALSE(utf8ById->empty());
}
