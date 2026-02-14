//
// kiwakiwaaにより 2026/02/12 に作成されました。
//

#include <gtest/gtest.h>
#include <filesystem>

#include "monokakido/dictionary/catalog.hpp"
#include "monokakido/core/platform/fs.hpp"
#include "monokakido/resource/headline/headline_store.hpp"
#include "monokakido/resource/keystore/keystore.hpp"
#include "../test_listener.hpp"

using namespace monokakido;

class HeadlineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto containerPath = platform::fs::getContainerPathByGroupIdentifier(MONOKAKIDO_GROUP_ID);
        const auto dictionariesPath = containerPath / DICTIONARIES_PATH;

        testHeadline_ = dictionariesPath / "KankenKJ2" / "Contents" / "KankenKJ2Data" / "headline" / "headline.headlinestore";
    }

    std::filesystem::path testHeadline_;
};


TEST_F(HeadlineTest, LoadValidHeadlineStore)
{
    auto headlineStoreResult = HeadlineStore::load(testHeadline_);
    ASSERT_TRUE(headlineStoreResult.has_value()) << "Failed to load headline store: " << headlineStoreResult.error();

    const auto& headlineStore = headlineStoreResult.value();

    test::verbosePrint("HeadlineStore Total Records: {}\n", headlineStore.size());
}