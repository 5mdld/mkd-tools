//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "support/test_dictionary.hpp"
#include <mutex>

namespace MKD::test
{
    using ExportTest = DictionaryTest;

    TEST_F(ExportTest, ExportsSelectedArticlesWithCompleteProgress)
    {
        Workspace workspace;
        std::mutex mutex;
        std::vector<ExportEvent> events;
        const auto output = workspace.path / "export";
        const auto result = require(exportToDirectory(birbs(), output, {.resources = {ResourceType::Contents}},
                                                      [&](const auto& event)
                                                      {
                                                          std::lock_guard lock(mutex);
                                                          events.push_back(event);
                                                      }));
        EXPECT_EQ(result.exported, 3);
        EXPECT_EQ(result.failed, 0);
        ASSERT_FALSE(events.empty());
        EXPECT_TRUE(std::holds_alternative<ExportBeginEvent>(events.front()));
        bool complete = false;
        for (const auto& event : events)
            if (const auto* progress = std::get_if<ProgressEvent>(&event))
                complete |= progress->completedItems == progress->totalItems && progress->totalItems == 3;
        EXPECT_TRUE(complete);
        size_t articles = 0;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(output))
        {
            if (entry.path().extension() != ".xml")
                continue;
            ++articles;
            const auto data = readFile(entry.path());
            EXPECT_NE(std::string(data.begin(), data.end()).find("<html"), std::string::npos);
        }
        EXPECT_EQ(articles, 3);
    }

    TEST_F(ExportTest, ExistingExportsAreSkippedUnlessOverwriteIsRequested)
    {
        Workspace workspace;
        const auto output = workspace.path / "export";
        ExportOptions options{.resources = {ResourceType::Contents}, .concurrency = 1};
        EXPECT_EQ(require(exportToDirectory(birbs(), output, options)).exported, 3);
        const auto repeated = require(exportToDirectory(birbs(), output, options));
        EXPECT_EQ(repeated.exported, 0);
        EXPECT_EQ(repeated.skipped, 3);
        options.overwriteExisting = true;
        EXPECT_EQ(require(exportToDirectory(birbs(), output, options)).exported, 3);
    }

    TEST_F(ExportTest, ExportsSaberlionAndBothKeystoreDirections)
    {
        Workspace workspace;
        const auto output = workspace.path / "export";
        const auto result = require(exportToDirectory(birbs(), output,
                                                      {
                                                          .resources = {
                                                              ResourceType::Graphics, ResourceType::Keystores
                                                          },
                                                          .keystoreExportMode = KeystoreExportMode::Both
                                                      }));
        EXPECT_EQ(result.failed, 0);
        EXPECT_EQ(readFile(output / "Graphics/saberlion.png"), readFile(assets() / "saberlion.png"));
        size_t indexes = 0;
        for (const auto& file : std::filesystem::directory_iterator(output / "Keystore"))
        {
            const auto data = readFile(file.path());
            EXPECT_FALSE(data.empty());
            ++indexes;
        }
        EXPECT_GE(indexes, 2);
    }

    TEST_F(ExportTest, InvalidOutputIsReportedWithoutReplacingTheFile)
    {
        Workspace workspace;
        const auto output = workspace.path / "perch";
        writeFile(output, bytes("occupied by a birb"));
        const auto result = exportToDirectory(birbs(), output, {.resources = {ResourceType::Contents}});
        EXPECT_TRUE(!result || result->failed > 0);
        EXPECT_EQ(readFile(output),
                  std::vector(bytes("occupied by a birb").begin(), bytes("occupied by a birb").end()));
    }
}
