//
// kiwakiwaaにより 2026/02/07 に作成されました。
//

#include <gtest/gtest.h>
#include <filesystem>

#include "../test_listener.hpp"
#include "../../src/platform/macos/fs.hpp"
#include "MKD/dictionary/dictionary_search.hpp"
#include "MKD/platform/macos/macos_dictionary_source.hpp"
#include "../../include/MKD/resource/keystore.hpp"

class KeystoreTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto containerPath = MKD::macOS::getContainerPathByGroupIdentifier(MKD::MONOKAKIDO_GROUP_ID);
        const auto dictionariesPath = containerPath / MKD::DICTIONARIES_SUBPATH;

        testHeadwords_ = dictionariesPath / "KNEJ" / "Contents" / "KNEJ" / "key" / "headword.keystore";
    }

    std::filesystem::path testHeadwords_;
};


TEST_F(KeystoreTest, LoadValidKeystoreFile)
{
    auto result2 = MKD::Keystore::open(testHeadwords_, "KNEJ");
    ASSERT_TRUE(result2.has_value()) << "Failed to load keystore: " << result2.error();

    auto& keystore = result2.value();
    ASSERT_TRUE(keystore.scope().has_value());
    EXPECT_EQ(*keystore.scope(), MKD::KeystoreScope::Headword);

    for (auto i = 0; i < keystore.indexSize(MKD::KeystoreIndex::Prefix); ++i)
    {
        auto lookupResult = keystore.keyAt(MKD::KeystoreIndex::Prefix, i);
        ASSERT_TRUE(lookupResult.has_value()) << "Failed to get key: " << lookupResult.error();
    }
}

TEST(KeystoreScopeTest, ParsesOriginalAppScopeAliases)
{
    EXPECT_EQ(MKD::parseKeystoreScope("headword"), MKD::KeystoreScope::Headword);
    EXPECT_EQ(MKD::parseKeystoreScope("index"), MKD::KeystoreScope::Headword);
    EXPECT_EQ(MKD::parseKeystoreScope("vocabulary"), MKD::KeystoreScope::Headword);
    EXPECT_EQ(MKD::parseKeystoreScope("jyukugo"), MKD::KeystoreScope::Idiom);
    EXPECT_EQ(MKD::parseKeystoreScope("kanyoku"), MKD::KeystoreScope::Idiom);
    EXPECT_EQ(MKD::parseKeystoreScope("yakugo"), MKD::KeystoreScope::Gogi);
    EXPECT_EQ(MKD::parseKeystoreScope("metadata"), MKD::KeystoreScope::Metadata);
    EXPECT_EQ(MKD::parseKeystoreScope("oyaji"), MKD::KeystoreScope::Kanji);
    EXPECT_EQ(MKD::parseKeystoreScope("cj"), MKD::KeystoreScope::CJ);
    EXPECT_EQ(MKD::parseKeystoreScope("jc"), MKD::KeystoreScope::JC);
    EXPECT_EQ(MKD::parseKeystoreScope("full-text"), MKD::KeystoreScope::Fulltext);
    EXPECT_EQ(MKD::parseKeystoreScope("group"), MKD::KeystoreScope::Group);
    EXPECT_EQ(MKD::parseKeystoreScope("compound noun"), MKD::KeystoreScope::CompoundNoun);
    EXPECT_EQ(MKD::parseKeystoreScopeFilename("jyukugo.keystore"), MKD::KeystoreScope::Idiom);
    EXPECT_EQ(MKD::parseKeystoreScopeFilename("metadata.keystore"), MKD::KeystoreScope::Metadata);
}

TEST(DictionarySearchScopeTest, ParsesOriginalAppScopeNames)
{
    EXPECT_EQ(MKD::searchScopeFromName("Headword"), MKD::SearchScope::Headword);
    EXPECT_EQ(MKD::searchScopeFromName("Index"), MKD::SearchScope::Headword);
    EXPECT_EQ(MKD::searchScopeFromName("Vocabulary"), MKD::SearchScope::Headword);
    EXPECT_EQ(MKD::searchScopeFromName("Idiom/Phrasal verb"), MKD::SearchScope::Idiom);
    EXPECT_EQ(MKD::searchScopeFromName("Jyukugo"), MKD::SearchScope::Idiom);
    EXPECT_EQ(MKD::searchScopeFromName("Yakugo"), MKD::SearchScope::Gogi);
    EXPECT_EQ(MKD::searchScopeFromName("Modern"), MKD::SearchScope::Modern);
    EXPECT_EQ(MKD::searchScopeFromName("MetaData"), MKD::SearchScope::Modern);
    EXPECT_EQ(MKD::searchScopeFromName("Oyaji"), MKD::SearchScope::Kanji);
    EXPECT_EQ(MKD::searchScopeFromName("CJ"), MKD::SearchScope::CJ);
    EXPECT_EQ(MKD::searchScopeFromName("JC"), MKD::SearchScope::JC);
    EXPECT_EQ(MKD::searchScopeFromName("Full-Text"), MKD::SearchScope::Fulltext);
    EXPECT_EQ(MKD::searchScopeFromName("Group"), MKD::SearchScope::Group);
    EXPECT_EQ(MKD::searchScopeFromName("Compound Noun"), MKD::SearchScope::CompoundNoun);
    EXPECT_EQ(MKD::searchScopeFromName("Numeral"), MKD::SearchScope::Numeral);
}
