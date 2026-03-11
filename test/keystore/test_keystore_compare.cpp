//
// kiwakiwaaにより 2026/03/10 に作成されました。
//

#include <gtest/gtest.h>
#include "../test_listener.hpp"
#include "../../../src/resource/keystore/keystore_compare.hpp"

using namespace MKD::detail;

class KeystoreCompareTest : public ::testing::Test
{
};


TEST_F(KeystoreCompareTest, IdenticalStringsAreEqual)
{
    EXPECT_EQ(keystore::compare("hello", "hello", keystore::CompareMode::Forward), 0);
    EXPECT_EQ(keystore::compare("hello", "hello", keystore::CompareMode::Backward), 0);
}

TEST_F(KeystoreCompareTest, EmptyStringsAreEqual)
{
    EXPECT_EQ(keystore::compare("", "", keystore::CompareMode::Forward), 0);
    EXPECT_EQ(keystore::compare("", "", keystore::CompareMode::Backward), 0);
}

TEST_F(KeystoreCompareTest, EmptyVsNonEmpty)
{
    EXPECT_EQ(keystore::compare("", "a", keystore::CompareMode::Forward), -1);
    EXPECT_EQ(keystore::compare("a", "", keystore::CompareMode::Forward), 1);
    EXPECT_EQ(keystore::compare("", "a", keystore::CompareMode::Backward), -1);
    EXPECT_EQ(keystore::compare("a", "", keystore::CompareMode::Backward), 1);
}

TEST_F(KeystoreCompareTest, LengthModeShorterFirst)
{
    EXPECT_EQ(keystore::compare("ab", "abc", keystore::CompareMode::Length), -1);
    EXPECT_EQ(keystore::compare("abc", "ab", keystore::CompareMode::Length), 1);
}

TEST_F(KeystoreCompareTest, LengthModeIgnoreablesNotCounted)
{
    EXPECT_EQ(keystore::compare("a b", "ab", keystore::CompareMode::Length), 0);
}

TEST_F(KeystoreCompareTest, LengthModeSameLengthFallsToForward)
{
    EXPECT_EQ(keystore::compare("abc", "abd", keystore::CompareMode::Length), -1);
    EXPECT_EQ(keystore::compare("abd", "abc", keystore::CompareMode::Length), 1);
    EXPECT_EQ(keystore::compare("abc", "abc", keystore::CompareMode::Length), 0);
}

TEST_F(KeystoreCompareTest, CaseInsensitiveAscii)
{
    EXPECT_EQ(keystore::compare("Hello", "hello", keystore::CompareMode::Forward), 0);
    EXPECT_EQ(keystore::compare("MEOW", "meow", keystore::CompareMode::Backward), 0);
}

TEST_F(KeystoreCompareTest, CaseInsensitiveUnicode)
{
    EXPECT_EQ(keystore::compare("Älg", "älg", keystore::CompareMode::Forward), 0);
    EXPECT_EQ(keystore::compare("Über", "über", keystore::CompareMode::Backward), 0);
}

TEST_F(KeystoreCompareTest, OnlyIgnorableCharsVsEmpty)
{
    EXPECT_EQ(keystore::compare("- -", "", keystore::CompareMode::Forward), 0);
    EXPECT_EQ(keystore::compare("   ", "", keystore::CompareMode::Forward), 0);
    EXPECT_EQ(keystore::compare("- -", "", keystore::CompareMode::Backward), 0);
}

TEST_F(KeystoreCompareTest, IdenticalKanji)
{
    EXPECT_EQ(keystore::compare("漢文", "漢文", keystore::CompareMode::Forward), 0);
    EXPECT_EQ(keystore::compare("漢文", "漢文", keystore::CompareMode::Backward), 0);
}

TEST_F(KeystoreCompareTest, KanaOrdering)
{
    EXPECT_EQ(keystore::compare("あ", "か", keystore::CompareMode::Forward), -1);
    EXPECT_EQ(keystore::compare("か", "あ", keystore::CompareMode::Forward), 1);
}


TEST_F(KeystoreCompareTest, CJKNotCaseFold)
{
    // kana should not be case folded
    EXPECT_NE(keystore::compare("あ", "ア", keystore::CompareMode::Forward), 0);
    EXPECT_NE(keystore::compare("あ", "ア", keystore::CompareMode::Backward), 0);
}


TEST_F(KeystoreCompareTest, CJKWithIgnorables)
{
    EXPECT_EQ(keystore::compare("豊葦-原瑞穂 国", "豊葦原瑞穂国", keystore::CompareMode::Forward), 0);
    EXPECT_EQ(keystore::compare("東京- 湾", "東京湾", keystore::CompareMode::Backward), 0);
}