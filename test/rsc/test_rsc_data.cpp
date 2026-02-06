//
// kiwakiwaaにより 2026/01/29 に作成されました。
//

#include <gtest/gtest.h>
#include <filesystem>
#include <algorithm>

#include "monokakido/core/platform/fs.hpp"
#include "monokakido/dictionary/catalog.hpp"
#include "monokakido/resource/rsc/rsc.hpp"
#include "monokakido/resource/xml_view.hpp"
#include "../common.hpp"

#include <pugixml.h>

class RscDataTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto containerPath = monokakido::platform::fs::getContainerPathByGroupIdentifier(
            monokakido::MONOKAKIDO_GROUP_ID);
        const auto dictionariesPath = containerPath / monokakido::DICTIONARIES_PATH;

        testDataPath_ = dictionariesPath / "KJT" / "Contents" / "KJT" / "contents";
        testFontDataPath_ = dictionariesPath / "KOGO3" / "Contents" / "ozk5" / "fonts" / "YuMinPr6N-R";
        dictId_ = "KJT";
    }

    std::filesystem::path testDataPath_;
    std::filesystem::path testFontDataPath_;
    std::string dictId_;
};


void printResourceFileInfo(const monokakido::RscResourceFile& file, size_t index)
{
    std::cout << std::format("  [{:2}] File: {}.rsc | Offset: {:10} | Length: {:10} | Path: {}\n",
                             index,
                             file.sequenceNumber,
                             file.globalOffset,
                             file.length,
                             file.filePath.filename().string());
}

TEST_F(RscDataTest, LoadValidRscData)
{
    auto result = monokakido::RscData::load(testDataPath_, dictId_);
    ASSERT_TRUE(result.has_value()) << "Failed to load RSC data: " << result.error();

    std::cout << "\n";
    monokakido::test::printSeparator();
    std::cout << std::format("RSC Data Loaded Successfully from: {}\n", testDataPath_.string());
    std::cout << std::format("Dictionary ID: {}\n", dictId_);
    monokakido::test::printSeparator();
    std::cout << "\n";
}


TEST_F(RscDataTest, GetRecordData)
{
    auto dataResult = monokakido::RscData::load(testDataPath_, dictId_);
    ASSERT_TRUE(dataResult.has_value());

    auto indexResult = monokakido::RscIndex::load(testDataPath_);
    ASSERT_TRUE(indexResult.has_value());

    auto& rscData = dataResult.value();
    const auto& index = indexResult.value();

    // Get first record
    auto recordResult = index.getByIndex(0);
    ASSERT_TRUE(recordResult.has_value());

    const auto& [itemId, mapRecord] = recordResult.value();

    auto dataSpan = rscData.get(mapRecord);
    ASSERT_TRUE(dataSpan.has_value()) << "Failed to get data: " << dataSpan.error();

    const auto& data = dataSpan.value();
    EXPECT_GT(data.size(), 0) << "Retrieved data should not be empty";

    const auto xmlResult = monokakido::XmlView{data}.asStringView();
    ASSERT_TRUE(xmlResult.has_value()) << "Data contains invalid UTF-8 sequence: " << xmlResult.error();

    pugi::xml_document xmlDoc;
    auto parseResult = xmlDoc.load_buffer(xmlResult.value().data(), xmlResult.value().size());
    ASSERT_TRUE(parseResult.status == pugi::status_ok) << std::format("Data contains invalid XML: {}", parseResult.description());
}


