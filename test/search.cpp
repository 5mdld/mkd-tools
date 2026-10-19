//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "support/test_dictionary.hpp"
#include "platform/search_lemma.hpp"
#include <future>
#include <tuple>

namespace MKD::test
{
    using SearchTest = DictionaryTest;

    TEST_F(SearchTest, FindsBirbsByExactPrefixSuffixAndNormalisedReading)
    {
        DictionarySearch search(birbs());
        EXPECT_TRUE(search.supportsNativeSearch());
        for (const auto& [query, mode] : std::vector<std::pair<std::string, SearchMode>>{
                 {"ツグミ", SearchMode::Exact}, {"つぐ", SearchMode::Prefix}, {"ぐみ", SearchMode::Suffix}
             })
        {
            const auto found = require(search.search(query, {.match = mode, .fallback = SearchFallback::None}));
            EXPECT_EQ(found.entries, (std::vector<EntryId>{{1, 0}}));
        }
        EXPECT_TRUE(require(search.search("not a birb")).empty());
        EXPECT_TRUE(require(search.search("")).empty());
        EXPECT_FALSE(search.search(std::string("\xff")));
    }

    TEST_F(SearchTest, ResolvesIdiomExampleSenseAndEnglishScopes)
    {
        DictionarySearch search(birbs());
        for (const auto& [scope, query, target] : std::vector<std::tuple<SearchScope, std::string, EntryId>>{
                 {SearchScope::Idiom, "いっせきにちょう", {1, 0x4001}},
                 {SearchScope::Example, "さえずる", {1, 0x5001}},
                 {SearchScope::Sense, "くいしんぼう", {3, 0xF001}},
                 {SearchScope::English, "thrush", {1, 0}}
             })
        {
            const auto found = require(search.search(query, {
                                                         .scope = scope, .match = SearchMode::Exact,
                                                         .fallback = SearchFallback::None
                                                     }));
            EXPECT_EQ(found.entries, (std::vector<EntryId>{target}));
            EXPECT_EQ(found.scope, scope);
            EXPECT_NE(require(birbs().entryUtf8ById(target.pageId)).find("<html"), std::string::npos);
        }
        EXPECT_TRUE(require(search.search("いっせき", {
                                              .scope = SearchScope::Idiom,
                                              .fallback = SearchFallback::None
                                          })).empty());
    }

    TEST_F(SearchTest, LimitsResultsAfterFilteringAndReportsTruncation)
    {
        DictionarySearch search(birbs());
        const auto all = require(search.search("birb", {.match = SearchMode::Exact}));
        EXPECT_EQ(all.entries, (std::vector<EntryId>{{1, 0}, {2, 0}}));
        EXPECT_FALSE(all.truncated);
        const auto limited = require(search.search("birb", {.match = SearchMode::Exact, .limit = 1}));
        EXPECT_EQ(limited.entries, (std::vector<EntryId>{{1, 0}}));
        EXPECT_TRUE(limited.truncated);
        const auto phrase = require(search.search("spotted birb", {.match = SearchMode::Exact}));
        EXPECT_EQ(phrase.entries, (std::vector<EntryId>{{1, 0}}));
    }

    TEST(Search, KanwaCompoundsPointInsideTheirParentArticle)
    {
        auto product = require(DictionaryProduct::openAtPath(dictionaries() / "BirbKanji"));
        EXPECT_EQ(product.metadata().productDelegateClass, "KLKanwaDicProductDelegate");
        EXPECT_TRUE(std::filesystem::exists(dictionaries() / "BirbKanji/Contents/kanji/key/jyukugo.keystore"));
        EXPECT_FALSE(std::filesystem::exists(dictionaries() / "BirbKanji/Contents/kanji/key/idiom.keystore"));
        const auto& dictionary = product.dictionaries().front();
        DictionarySearch search(dictionary);
        EXPECT_TRUE(search.supportsNativeSearch());
        const auto found = require(search.search("ちょうご", {
                                                     .scope = SearchScope::Idiom,
                                                     .match = SearchMode::Exact
                                                 }));
        EXPECT_EQ(found.entries, (std::vector<EntryId>{{1, 0x8001}}));
        EXPECT_NE(require(dictionary.entryUtf8ById(1)).find("00001-8001"), std::string::npos);
        EXPECT_EQ(require(dictionary.entryOrder().next(found.entries.front().pageId)), std::nullopt);
    }

    TEST_F(SearchTest, ConcurrentQueriesHaveIndependentCancellation)
    {
        DictionarySearch search(birbs());
        std::stop_source stop;
        stop.request_stop();
        const auto cancelled = search.search("birb", {}, stop.get_token());
        EXPECT_FALSE(cancelled);
        auto first = std::async(std::launch::async, [&] { return search.search("つぐみ"); });
        auto second = std::async(std::launch::async, [&] { return search.search("すずめ"); });
        EXPECT_EQ(require(first.get()).entries, (std::vector<EntryId>{{1, 0}}));
        EXPECT_EQ(require(second.get()).entries, (std::vector<EntryId>{{2, 0}}));
    }

    TEST(Search, IntersectsHeadlineIdentifiersAfterResolvingTheirTargets)
    {
        Workspace workspace;
        const auto path = workspace.path / "headword.keystore";
        auto writer = require(KeystoreWriter::create(path, {
                                                         .conversions = {
                                                             {0, 0}, {7, 0x8001}, {9, 0}, {7, 0x8001}, {9, 0},
                                                             {7, 0x8002}
                                                         }
                                                     }));
        require(writer.add("small", {1, 0}));
        require(writer.add("small", {2, 0}));
        require(writer.add("birb", {3, 0}));
        require(writer.add("birb", {4, 0}));
        require(writer.add("wing", {5, 0}));
        require(writer.finish());
        DictionaryResources resources;
        resources.keystores.push_back(require(Keystore::open(path)));
        auto content = birbContent();
        content.hasHeadlineKeyIdentifier = true;
        Dictionary dictionary(content, std::move(resources));
        DictionarySearch search(dictionary);
        const auto found = require(search.search("small birb", {.limit = 1}));
        EXPECT_EQ(found.entries, (std::vector<EntryId>{{7, 0x8001}}));
        EXPECT_TRUE(found.truncated);
        EXPECT_EQ(found.matches.size(), 2);
        EXPECT_TRUE(require(search.search("absent birb")).empty());
        EXPECT_TRUE(require(search.search("wing birb")).empty());
        EXPECT_TRUE(require(search.search("small birb", {.fallback = SearchFallback::None})).empty());
    }

    TEST(Search, UnknownNativeProfilesFailRatherThanInventingSearchRules)
    {
        Dictionary dictionary(birbContent(), {}, {.productSearchClass = "UnknownBirbSearch"});
        DictionarySearch search(dictionary);
        EXPECT_FALSE(search.supportsNativeSearch());
        EXPECT_FALSE(search.search("birb"));
    }

    TEST(Search, LinguisticSupportReflectsThePlatform)
    {
#ifdef __APPLE__
        EXPECT_EQ(detail::searchLemma("cats", "en"), "cat");
#else
        EXPECT_EQ(detail::searchLemma("cats", "en"), std::nullopt);
#endif
    }
}
