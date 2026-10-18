//
// kiwakiwaaにより 2026/02/07 に作成されました。
//

#include <gtest/gtest.h>
#include <filesystem>

#include "../test_listener.hpp"
#include "MKD/dictionary/dictionary_search.hpp"
#include "MKD/platform/macos/paths.hpp"
#include "../../include/MKD/resource/keystore.hpp"

class KeystoreTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        auto containerPath = MKD::macOS::monokakidoDataDirectory();
        ASSERT_TRUE(containerPath) << containerPath.error();
        const auto dictionariesPath = MKD::macOS::dictionariesDirectory(*containerPath);

        testHeadwords_ = dictionariesPath / "KNEJ" / "Contents" / "KNEJ" / "key" / "headword.keystore";
    }

    std::filesystem::path testHeadwords_;
};


TEST_F(KeystoreTest, LoadValidKeystoreFile)
{
    auto result2 = MKD::Keystore::open(testHeadwords_);
    ASSERT_TRUE(result2.has_value()) << "Failed to load keystore: " << result2.error();

    auto& keystore = result2.value();
    ASSERT_TRUE(keystore.scope().has_value());
    EXPECT_EQ(*keystore.scope(), MKD::SearchScope::Headword);

    for (auto i = 0; i < keystore.indexSize(MKD::KeystoreIndex::Prefix); ++i)
    {
        auto lookupResult = keystore.keyAt(MKD::KeystoreIndex::Prefix, i);
        ASSERT_TRUE(lookupResult.has_value()) << "Failed to get key: " << lookupResult.error();
    }
}

TEST_F(KeystoreTest, AppliesEmbeddedConversionTable)
{
    auto result = MKD::Keystore::open(testHeadwords_);
    ASSERT_TRUE(result.has_value()) << "Failed to load keystore: " << result.error();

    auto entryIds = result->entryIdsAt(MKD::KeystoreIndex::Prefix, 0);
    ASSERT_TRUE(entryIds.has_value()) << "Failed to decode entry ids: " << entryIds.error();
    ASSERT_FALSE(entryIds->empty());

    EXPECT_EQ(entryIds->front().pageId, 160466u);
    EXPECT_EQ(entryIds->front().itemId, 0u);
}

TEST(SearchScopeTest, ParsesOriginalAppScopeAliases)
{
    EXPECT_EQ(MKD::searchScopeFromName("headword"), MKD::SearchScope::Headword);
    EXPECT_EQ(MKD::searchScopeFromName("index"), MKD::SearchScope::Headword);
    EXPECT_EQ(MKD::searchScopeFromName("vocabulary"), MKD::SearchScope::Headword);
    EXPECT_EQ(MKD::searchScopeFromName("jyukugo"), MKD::SearchScope::Idiom);
    EXPECT_EQ(MKD::searchScopeFromName("kanyoku"), MKD::SearchScope::Idiom);
    EXPECT_EQ(MKD::searchScopeFromName("yakugo"), MKD::SearchScope::Sense);
    EXPECT_EQ(MKD::searchScopeFromName("metadata"), MKD::SearchScope::Modern);
    EXPECT_EQ(MKD::searchScopeFromName("oyaji"), MKD::SearchScope::Kanji);
    EXPECT_EQ(MKD::searchScopeFromName("cj"), MKD::SearchScope::CJ);
    EXPECT_EQ(MKD::searchScopeFromName("jc"), MKD::SearchScope::JC);
    EXPECT_EQ(MKD::searchScopeFromName("full-text"), MKD::SearchScope::Fulltext);
    EXPECT_EQ(MKD::searchScopeFromName("group"), MKD::SearchScope::Category);
    EXPECT_EQ(MKD::searchScopeFromName("compound noun"), MKD::SearchScope::CompoundNoun);
    EXPECT_EQ(MKD::searchScopeFromFilename("jyukugo.keystore"), MKD::SearchScope::Idiom);
    EXPECT_EQ(MKD::searchScopeFromFilename("metadata.keystore"), MKD::SearchScope::Modern);
}

TEST(DictionarySearchScopeTest, ParsesOriginalAppScopeNames)
{
    EXPECT_EQ(MKD::searchScopeFromName("Headword"), MKD::SearchScope::Headword);
    EXPECT_EQ(MKD::searchScopeFromName("Index"), MKD::SearchScope::Headword);
    EXPECT_EQ(MKD::searchScopeFromName("Vocabulary"), MKD::SearchScope::Headword);
    EXPECT_EQ(MKD::searchScopeFromName("Idiom/Phrasal verb"), MKD::SearchScope::Idiom);
    EXPECT_EQ(MKD::searchScopeFromName("Jyukugo"), MKD::SearchScope::Idiom);
    EXPECT_EQ(MKD::searchScopeFromName("Yakugo"), MKD::SearchScope::Sense);
    EXPECT_EQ(MKD::searchScopeFromName("Modern"), MKD::SearchScope::Modern);
    EXPECT_EQ(MKD::searchScopeFromName("MetaData"), MKD::SearchScope::Modern);
    EXPECT_EQ(MKD::searchScopeFromName("Oyaji"), MKD::SearchScope::Kanji);
    EXPECT_EQ(MKD::searchScopeFromName("CJ"), MKD::SearchScope::CJ);
    EXPECT_EQ(MKD::searchScopeFromName("JC"), MKD::SearchScope::JC);
    EXPECT_EQ(MKD::searchScopeFromName("Full-Text"), MKD::SearchScope::Fulltext);
    EXPECT_EQ(MKD::searchScopeFromName("Group"), MKD::SearchScope::Category);
    EXPECT_EQ(MKD::searchScopeFromName("Compound Noun"), MKD::SearchScope::CompoundNoun);
    EXPECT_EQ(MKD::searchScopeFromName("Numeral"), MKD::SearchScope::Numeral);
}
