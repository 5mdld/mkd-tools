//
// kiwakiwaaにより 2026/03/13 に作成されました。
//

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <stdexcept>

#include "../test_listener.hpp"
#include "MKD/dictionary/dictionary_product.hpp"
#include "MKD/platform/macos/paths.hpp"
#include "MKD/dictionary/dictionary_search.hpp"

using namespace MKD;

class DictionarySearchTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        auto containerPath = MKD::macOS::monokakidoDataDirectory();
        ASSERT_TRUE(containerPath) << containerPath.error();
        dictionariesPath_ = MKD::macOS::dictionariesDirectory(*containerPath);
    }

    [[nodiscard]] Dictionary loadDictionary(const std::string& id) const
    {
        const auto dictPath = dictionariesPath_ / id;
        auto product = DictionaryProduct::openAtPath(dictPath);
        if (!product)
            throw std::runtime_error(product.error());

        auto& dicts = product->dictionaries();
        if (dicts.empty())
            throw std::runtime_error("Product contains no dictionaries: " + dictPath.string());

        return std::move(dicts.front());
    }

    static void printInfo(const std::vector<SearchMatch>& keys, const std::vector<EntryId>& entryIds)
    {
        for (auto& key : keys)
        {
            test::verbosePrint("{}, ", key.key);
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
        const auto start = std::chrono::high_resolution_clock::now();
        auto result = searchFunc();
        const auto end = std::chrono::high_resolution_clock::now();

        const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double milliseconds = static_cast<double>(duration.count()) / 1000.0;

        test::verbosePrint("⏱ {} search took: {:.3f} ms ({} μs)\n",
                           searchName, milliseconds, duration.count());

        return result;
    }

    std::filesystem::path dictionariesPath_;
};

TEST_F(DictionarySearchTest, KANKENKJ2_KanjiHeadwordSearch)
{
    const auto dict = loadDictionary("KANKENKJ2");
    const DictionarySearch search(dict);

    const auto result = measureSearchTime("KANKENKJ2 headword", [&]() {
        return search.search("愛");
    });

    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->empty());
    printInfo(result->matches, result->entries);
}

TEST_F(DictionarySearchTest, KANKENKJ2_CompoundKanjiSearch)
{
    const auto dict = loadDictionary("KANKENKJ2");
    const DictionarySearch search(dict);

    const auto result = measureSearchTime("KANKENKJ2 compound", [&]() {
        return search.search("愛情");
    });

    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->empty());

    ASSERT_FALSE(result->matches.empty());
    printInfo(result->matches, result->entries);
}

TEST_F(DictionarySearchTest, KANKENKJ2_PrefixSearch)
{
    const auto dict = loadDictionary("KANKENKJ2");
    const DictionarySearch search(dict);

    SearchOptions options;
    options.match = SearchMode::Prefix;
    options.scope = SearchScope::Idiom;

    const auto result = measureSearchTime("KANKENKJ2 prefix", [&]() {
        return search.search("愛", options);
    });

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->entries.size() == 43);
    ASSERT_FALSE(result->empty());
    printInfo(result->matches, result->entries);
}

TEST_F(DictionarySearchTest, OKO12_HeadwordSearch)
{
    const auto dict = loadDictionary("KOGO3");
    const DictionarySearch search(dict);

    const auto result = measureSearchTime("KOGO3 headword", [&]() {
        return search.search("学生");
    });

    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->empty());
    printInfo(result->matches, result->entries);
}

TEST_F(DictionarySearchTest, SHINJIGEN_GogiSearch)
{
    const auto dict = loadDictionary("SHINJIGEN2");
    const DictionarySearch search(dict);

    SearchOptions options;
    options.match = SearchMode::Prefix;
    options.scope = SearchScope::Sense;

    const auto result = measureSearchTime("Shinjigen Gogi search", [&]() {
        return search.search("ひよどり", options);
    });

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->entries.size() == 4);
    ASSERT_FALSE(result->empty());

    printInfo(result->matches, result->entries);
}

TEST_F(DictionarySearchTest, OKO12_JoinedQueryFallback)
{
    const auto dict = loadDictionary("KOGO3");
    const DictionarySearch search(dict);

    const auto result = measureSearchTime("KOGO3 joined query", [&]() {
        return search.search("取り 扱い");
    });

    ASSERT_TRUE(result.has_value());
    printInfo(result->matches, result->entries);
}

TEST_F(DictionarySearchTest, KOGO3_HeadwordSearch)
{
    const auto dict = loadDictionary("SKOGO");
    const DictionarySearch search(dict);

    const auto result = measureSearchTime("KOGO3 headword", [&]() {
        return search.search("かわご");
    });

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->entries.size() == 1);
    ASSERT_FALSE(result->empty());
    printInfo(result->matches, result->entries);
}

TEST_F(DictionarySearchTest, SKOGO_ModernSearch)
{
    const auto dict = loadDictionary("SKOGO");
    const DictionarySearch search(dict);

    SearchOptions options;
    options.scope = SearchScope::Modern;

    const auto result = measureSearchTime("SKOGO modern", [&]() {
        return search.search("あう", options);
    });

    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->empty());
    EXPECT_EQ(result->scope, SearchScope::Modern);
    printInfo(result->matches, result->entries);
}

TEST_F(DictionarySearchTest, SKOGO_GendaiHeadwordSort)
{
    const auto dict = loadDictionary("SKOGO");
    const DictionarySearch search(dict);

    const auto result = measureSearchTime("SKOGO gendai headword", [&]() {
        return search.search("あう");
    });

    for (auto& entry : result->entries)
    {
        auto headline = dict.headlineForEntryId(entry);
        ASSERT_TRUE(headline.has_value());
    }

    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->empty());
    EXPECT_GE(result->entries.front().pageId, 19249);
    EXPECT_LE(result->entries.front().pageId, 25337);
    ASSERT_GE(result->entries.size(), 2);
    EXPECT_LT(result->entries[1].pageId, 19249);
    printInfo(result->matches, result->entries);
}

TEST_F(DictionarySearchTest, OKG11_HeadwordSearch)
{
    const auto dict = loadDictionary("OKG11");
    const DictionarySearch search(dict);

    const auto result = measureSearchTime("OKG11 headword", [&]() {
        return search.search("あはれ");
    });

    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->empty());
    printInfo(result->matches, result->entries);
}

TEST_F(DictionarySearchTest, CancellationBelongsToOneQuery)
{
    const auto dict = loadDictionary("KOGO3");
    const DictionarySearch search(dict);

    std::stop_source cancellation;
    cancellation.request_stop();
    EXPECT_FALSE(search.search("日本", {}, cancellation.get_token()));
    EXPECT_TRUE(search.search("日本"));
}

TEST_F(DictionarySearchTest, DictionaryEntryAccessHelpers)
{
    auto dict = loadDictionary("KOGO3");
    ASSERT_TRUE(dict.hasEntries());

    auto firstEntry = dict.entryByIndex(0);
    ASSERT_TRUE(firstEntry.has_value());
    EXPECT_FALSE(firstEntry->data.empty());

    auto utf8View = firstEntry->data.utf8();
    ASSERT_TRUE(utf8View.has_value());
    EXPECT_FALSE(utf8View->empty());

    auto sameEntry = dict.entryById(firstEntry->itemId);
    ASSERT_TRUE(sameEntry.has_value());

    auto utf8ById = dict.entryUtf8ById(firstEntry->itemId);
    ASSERT_TRUE(utf8ById.has_value());
    EXPECT_FALSE(utf8ById->empty());
}
