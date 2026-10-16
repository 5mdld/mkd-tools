//
// kiwakiwaaにより 2026/02/12 に作成されました。
//

#include <gtest/gtest.h>
#include <filesystem>

#include "../test_listener.hpp"
#include "MKD/platform/macos/paths.hpp"
#include "../../include/MKD/resource/headline_store.hpp"

using namespace MKD;

class HeadlineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        auto containerPath = MKD::macOS::monokakidoDataDirectory();
        ASSERT_TRUE(containerPath) << containerPath.error();
        const auto dictionariesPath = MKD::macOS::dictionariesDirectory(*containerPath);

        testHeadline_ = dictionariesPath / "KankenKJ2" / "Contents" / "KankenKJ2Data" / "headline" / "headline.headlinestore";
    }

    std::filesystem::path testHeadline_;
};


TEST_F(HeadlineTest, LoadValidHeadlineStore)
{
    auto headlineStoreResult = HeadlineStore::open(testHeadline_);
    ASSERT_TRUE(headlineStoreResult.has_value()) << "Failed to load headline store: " << headlineStoreResult.error();

    const auto& headlineStore = headlineStoreResult.value();

    test::verbosePrint("HeadlineStore Total Records: {}\n", headlineStore.size());
}