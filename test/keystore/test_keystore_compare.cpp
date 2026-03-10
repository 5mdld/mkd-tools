//
// kiwakiwaaにより 2026/03/10 に作成されました。
//

#include <gtest/gtest.h>
#include "../test_listener.hpp"
#include "../../../src/resource/keystore/keystore_search.hpp"

class KeystoreCompareTest : public ::testing::Test
{
};


TEST_F(KeystoreCompareTest, IdenticalStringsAreEqual)
{
    EXPECT_EQ(MKD::keystoreCompare("hello", "hello", MKD::CompareMode::Forward), 0);
    EXPECT_EQ(MKD::keystoreCompare("hello", "hello", MKD::CompareMode::Backward), 0);
}

TEST_F(KeystoreCompareTest, EmptyStringsAreEqual)
{
    EXPECT_EQ(MKD::keystoreCompare("", "", MKD::CompareMode::Forward), 0);
    EXPECT_EQ(MKD::keystoreCompare("", "", MKD::CompareMode::Backward), 0);
}

TEST_F(KeystoreCompareTest, EmptyVsNonEmpty)
{
    EXPECT_EQ(MKD::keystoreCompare("", "a", MKD::CompareMode::Forward), -1);
    EXPECT_EQ(MKD::keystoreCompare("a", "", MKD::CompareMode::Forward), 1);
    EXPECT_EQ(MKD::keystoreCompare("", "a", MKD::CompareMode::Backward), -1);
    EXPECT_EQ(MKD::keystoreCompare("a", "", MKD::CompareMode::Backward), 1);
}

TEST_F(KeystoreCompareTest, LengthModeShorterFirst)
{
    EXPECT_EQ(MKD::keystoreCompare("ab", "abc", MKD::CompareMode::Length), -1);
    EXPECT_EQ(MKD::keystoreCompare("abc", "ab", MKD::CompareMode::Length), 1);
}

TEST_F(KeystoreCompareTest, LengthModeIgnoreablesNotCounted)
{
    EXPECT_EQ(MKD::keystoreCompare("a b", "ab", MKD::CompareMode::Length), 0);
}

TEST_F(KeystoreCompareTest, LengthModeSameLengthFallsToForward)
{
    EXPECT_EQ(MKD::keystoreCompare("abc", "abd", MKD::CompareMode::Length), -1);
    EXPECT_EQ(MKD::keystoreCompare("abd", "abc", MKD::CompareMode::Length), 1);
    EXPECT_EQ(MKD::keystoreCompare("abc", "abc", MKD::CompareMode::Length), 0);
}

TEST_F(KeystoreCompareTest, CaseInsensitiveAscii)
{
    EXPECT_EQ(MKD::keystoreCompare("Hello", "hello", MKD::CompareMode::Forward), 0);
    EXPECT_EQ(MKD::keystoreCompare("MEOW", "meow", MKD::CompareMode::Backward), 0);
}

TEST_F(KeystoreCompareTest, CaseInsensitiveUnicode)
{
    EXPECT_EQ(MKD::keystoreCompare("Älg", "älg", MKD::CompareMode::Forward), 0);
    EXPECT_EQ(MKD::keystoreCompare("Über", "über", MKD::CompareMode::Backward), 0);
}

TEST_F(KeystoreCompareTest, OnlyIgnorableCharsVsEmpty)
{
    EXPECT_EQ(MKD::keystoreCompare("- -", "", MKD::CompareMode::Forward), 0);
    EXPECT_EQ(MKD::keystoreCompare("   ", "", MKD::CompareMode::Forward), 0);
    EXPECT_EQ(MKD::keystoreCompare("- -", "", MKD::CompareMode::Backward), 0);
}

TEST_F(KeystoreCompareTest, IdenticalKanji)
{
    EXPECT_EQ(MKD::keystoreCompare("漢文", "漢文", MKD::CompareMode::Forward), 0);
    EXPECT_EQ(MKD::keystoreCompare("漢文", "漢文", MKD::CompareMode::Backward), 0);
}

TEST_F(KeystoreCompareTest, KanaOrdering)
{
    EXPECT_EQ(MKD::keystoreCompare("あ", "か", MKD::CompareMode::Forward), -1);
    EXPECT_EQ(MKD::keystoreCompare("か", "あ", MKD::CompareMode::Forward), 1);
}


TEST_F(KeystoreCompareTest, CJKNotCaseFold)
{
    // kana should not be case folded
    EXPECT_NE(MKD::keystoreCompare("あ", "ア", MKD::CompareMode::Forward), 0);
    EXPECT_NE(MKD::keystoreCompare("あ", "ア", MKD::CompareMode::Backward), 0);
}


TEST_F(KeystoreCompareTest, CJKWithIgnorables)
{
    EXPECT_EQ(MKD::keystoreCompare("豊葦-原瑞穂 国", "豊葦原瑞穂国", MKD::CompareMode::Forward), 0);
    EXPECT_EQ(MKD::keystoreCompare("東京- 湾", "東京湾", MKD::CompareMode::Backward), 0);
}