//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "support/test_dictionary.hpp"
#include <array>
#include <algorithm>
#include <map>

namespace MKD::test
{
    TEST(Creation, PublishesOnlyAfterEveryDictionaryIsFinished)
    {
        Workspace workspace;
        const auto path = workspace.path / "Birbs";
        BirbBuild build(path);
        const auto target = build.addBirb();
        EXPECT_FALSE(std::filesystem::exists(path));
        build.finish();
        auto product = require(DictionaryProduct::openAtPath(path));
        EXPECT_EQ(require(DictionarySearch(product.dictionaries().front()).search("すずめ")).entries,
                  (std::vector<EntryId>{target}));
        EXPECT_FALSE(build.dictionary.createEntry());
        EXPECT_FALSE(build.product.finish());
    }

    TEST(Creation, AbandonedBuildsLeaveNoPublishedDictionary)
    {
        Workspace workspace;
        const auto path = workspace.path / "LostBirb";
        {
            BirbBuild build(path);
            build.addBirb();
        }
        EXPECT_FALSE(std::filesystem::exists(path));
        EXPECT_TRUE(std::filesystem::is_empty(workspace.path));
    }

    TEST(Creation, ExistingProductsAreNeverReplaced)
    {
        Workspace workspace;
        const auto path = workspace.path / "Birbs";
        BirbBuild build(path);
        build.addBirb();
        build.finish();
        EXPECT_FALSE(ProductWriter::create(path, {}));
        EXPECT_TRUE(DictionaryProduct::openAtPath(path));
    }

    TEST(Creation, AllocatesTargetsAndReopensSparseBrowsingOrder)
    {
        Workspace workspace;
        BirbBuild build(workspace.path / "Birbs", true);
        auto entry = require(build.dictionary.createEntry(7));
        const auto compound = require(entry.addTarget(EntryTargetKind::Compound));
        const auto example = require(entry.addTarget(EntryTargetKind::Example));
        EXPECT_EQ(compound, (EntryId{7, 0x8001}));
        EXPECT_EQ(example, (EntryId{7, 0x5001}));
        require(entry.writeXml("<html><body><p id=\"" + compound.toString() + "\">雀躍</p><p id=\"" +
            example.toString() + "\">雀が跳ねる。</p></body></html>"));
        require(build.dictionary.addHeadline({.headline = u"雀", .entryId = entry.target()}));
        require(build.dictionary.addSearchKey(SearchScope::Headword, "すずめ", entry.target()));
        const auto second = build.addBirb();
        require(build.dictionary.setEntryOrder(std::array{second.pageId, uint32_t{7}}));
        build.finish();
        auto product = require(DictionaryProduct::openAtPath(workspace.path / "Birbs"));
        const auto& order = product.dictionaries().front().entryOrder();
        EXPECT_EQ(require(order.pageAt(0)), second.pageId);
        EXPECT_EQ(require(order.next(second.pageId)), 7);
        EXPECT_EQ(require(order.next(7)), std::nullopt);
        EXPECT_FALSE(entry.addTarget(EntryTargetKind::Sense));
    }

    TEST(Creation, MissingAnchorsPreventPublication)
    {
        Workspace workspace;
        BirbBuild build(workspace.path / "Birbs");
        auto entry = require(build.dictionary.createEntry());
        require(entry.addTarget(EntryTargetKind::Idiom));
        require(entry.writeXml("<html><body>No perch for this target.</body></html>"));
        require(build.dictionary.addHeadline({.headline = u"雀", .entryId = entry.target()}));
        require(build.dictionary.addSearchKey(SearchScope::Headword, "すずめ", entry.target()));
        const auto result = build.dictionary.finish();
        ASSERT_FALSE(result);
        EXPECT_NE(result.error().find("Missing entry target"), std::string::npos);
        EXPECT_FALSE(build.product.finish());
        EXPECT_FALSE(std::filesystem::exists(workspace.path / "Birbs"));
    }

    TEST(Creation, MissingAssetsAndEscapingPathsAreRejected)
    {
        Workspace workspace;
        BirbBuild build(workspace.path / "Birbs");
        EXPECT_FALSE(build.dictionary.addAsset("../escaped.css", bytes("body{}")));
        // A failed write poisons its build, so check unresolved assets in a fresh one.
        BirbBuild missing(workspace.path / "MissingBirb");
        auto entry = require(missing.dictionary.createEntry());
        require(entry.writeXml("<html>\n<body><img src=\"graphics/saberlion.png\"/></body></html>"));
        require(missing.dictionary.addHeadline({.headline = u"獅子", .entryId = entry.target()}));
        require(missing.dictionary.addSearchKey(SearchScope::Headword, "saberlion", entry.target()));
        const auto result = missing.dictionary.finish();
        ASSERT_FALSE(result);
        EXPECT_NE(result.error().find("page 1 at byte "), std::string::npos);
        EXPECT_NE(result.error().find("line 2"), std::string::npos);
        EXPECT_FALSE(std::filesystem::exists(workspace.path / "MissingBirb"));
    }

    TEST(Creation, SourceLocationsReachImmediateAndDeferredErrors)
    {
        Workspace workspace;
        BirbBuild syntax(workspace.path / "Syntax");
        const auto malformed = syntax.dictionary.addAsset("article.css", bytes("p{}\r\n/* unfinished"));
        ASSERT_FALSE(malformed);
        EXPECT_NE(malformed.error().find("article.css at byte 5 (line 2, byte column 1)"), std::string::npos);

        BirbBuild paths(workspace.path / "Paths");
        const auto invalid = paths.dictionary.addAsset("article.css", bytes("p{}\np { src:url(bad%zz); }"));
        ASSERT_FALSE(invalid);
        EXPECT_NE(invalid.error().find("article.css at byte 12 (line 2, byte column 9)"), std::string::npos);

        BirbBuild missing(workspace.path / "Missing");
        missing.addBirb();
        require(missing.dictionary.addAsset("article.css", bytes("p {}\r\np { fill: url(missing.svg); }")));
        const auto unresolved = missing.dictionary.finish();
        ASSERT_FALSE(unresolved);
        EXPECT_NE(unresolved.error().find("article.css at byte 16 (line 2, byte column 11)"), std::string::npos);
        EXPECT_FALSE(std::filesystem::exists(workspace.path / "Missing"));
    }

    TEST(Creation, DuplicatePagesAndIncompleteOrdersAreRejected)
    {
        Workspace workspace;
        BirbBuild build(workspace.path / "Birbs");
        build.addBirb(1);
        EXPECT_FALSE(build.dictionary.createEntry(1));
        BirbBuild order(workspace.path / "Order");
        order.addBirb(1);
        order.addBirb(7);
        require(order.dictionary.setEntryOrder(std::array{uint32_t{1}}));
        EXPECT_FALSE(order.dictionary.finish());
        EXPECT_FALSE(order.product.finish());
    }

    TEST(Creation, InvalidXmlAndForeignTargetsAreRejected)
    {
        Workspace workspace;
        for (const auto& [name, xml] : std::array<std::pair<std::string, std::string>, 3>{
                 std::pair{"broken", "<html><body>"},
                 {"foreign", "<html><body id=\"00099-8001\">A lost birb.</body></html>"},
                 {"unicode", std::string("<html><body>\xff</body></html>")}
             })
        {
            BirbBuild build(workspace.path / name);
            auto entry = require(build.dictionary.createEntry());
            EXPECT_FALSE(entry.writeXml(xml));
            EXPECT_FALSE(build.product.finish());
        }
    }

    TEST(Creation, UnfinishedContentsPreventTheProductFromPublishing)
    {
        Workspace workspace;
        BirbBuild build(workspace.path / "Birbs");
        build.addBirb();
        EXPECT_FALSE(build.product.finish());
        EXPECT_FALSE(std::filesystem::exists(workspace.path / "Birbs"));
    }

    TEST(Creation, AppendixMenusRoundTripAcrossDictionaries)
    {
        Workspace workspace;
        const auto path = workspace.path / "Birbs";
        BirbBuild build(path, true);
        const auto birb = build.addBirb();
        auto secondContent = birbContent();
        secondContent.identifier = "test.saberlion";
        secondContent.directory = "lion";
        secondContent.dictionaryId = 1001;
        auto lion = require(build.product.addDictionary(secondContent));
        auto entry = require(lion.createEntry());
        const auto target = require(entry.addTarget(EntryTargetKind::Sense));
        require(entry.writeXml("<html><body><p id=\"" + target.toString() + "\">がおー</p></body></html>"));
        require(lion.addHeadline({.headline = u"獅子", .entryId = entry.target()}));
        require(lion.addSearchKey(SearchScope::Headword, "がおー", entry.target()));
        const auto list = require(build.dictionary.addAppendix("birbs", "小鳥", std::array{
                                                                   AppendixEntry{birb},
                                                                   AppendixEntry{
                                                                       target, std::nullopt, std::nullopt, 1001
                                                                   }
                                                               }));
        require(build.product.setAppendices({
            {
                {
                    "仲間", {
                        {"小鳥", list}, {"獅子", AppendixEntryLink{target, 1001}},
                    }
                }
            }
        }));
        const auto report = require(build.product.preflight());
        EXPECT_TRUE(report.valid());
        EXPECT_EQ(report.pendingDictionaries.size(), 2);
        require(lion.finish());
        EXPECT_TRUE(require(build.product.preflight()).valid());
        build.finish();
        auto product = require(DictionaryProduct::openAtPath(path));
        const auto& items = product.appendices().sections.front().items;
        EXPECT_EQ(items.front().name, "小鳥");
        EXPECT_EQ(require(product.openAppendixList(std::get<AppendixList>(items.front().target))).size(), 2);
        const auto link = std::get<AppendixEntryLink>(items.back().target);
        const auto* owner = product.findContent(link.dictionaryId);
        ASSERT_NE(owner, nullptr);
        EXPECT_EQ(link.entryId, target);
        EXPECT_NE(require(owner->entryUtf8ById(target.pageId)).find("がおー"), std::string::npos);
    }

    TEST(Creation, MissingAppendixTargetsPreventPublication)
    {
        Workspace workspace;
        for (const auto& [name, target] : std::array<std::pair<std::string, AppendixTarget>, 3>{
                 std::pair{"document", AppendixDocument{"missing.html"}},
                 {"dictionary", AppendixEntryLink{{1, 0}, 999}},
                 {"anchor", AppendixEntryLink{{1, 0x8001}, 1000}},
             })
        {
            BirbBuild build(workspace.path / name);
            build.addBirb();
            require(build.product.setAppendices({{{"", {{"Lost birb", target}}}}}));
            require(build.dictionary.finish());
            EXPECT_FALSE(build.product.finish());
            EXPECT_FALSE(std::filesystem::exists(workspace.path / name));
        }
    }
    TEST(Creation, PreflightAndCompletionValidationLeaveSupportedRepairsOpen)
    {
        Workspace workspace;
        const auto path = workspace.path / "Repair";
        BirbBuild build(path);
        auto entry = require(build.dictionary.createEntry(7, {.filename = "birds.xml", .record = "sparrow"}));
        auto reserved = require(build.dictionary.createEntry(9));
        require(entry.writeXml("<html>\n<body><img src=\"missing.png\"/></body></html>"));
        require(build.dictionary.addSearchKey(SearchScope::Headword, "sparrow", entry.target()));
        const auto snapshot = [&]
        {
            std::map<std::filesystem::path, std::vector<uint8_t>> files;
            for (const auto& file : std::filesystem::recursive_directory_iterator(workspace.path))
                files.emplace(file.path(), file.is_regular_file() ? readFile(file.path()) : std::vector<uint8_t>{});
            return files;
        };
        const auto before = snapshot();
        const auto report = require(build.dictionary.preflight());
        EXPECT_FALSE(report.valid());
        EXPECT_GE(report.totalIssues, 4);
        const auto limited = require(build.dictionary.preflight({.maxIssues = 0}));
        EXPECT_TRUE(limited.issues.empty());
        EXPECT_TRUE(limited.truncated());
        EXPECT_FALSE(limited.valid());
        EXPECT_EQ(report.describe(), require(build.dictionary.preflight()).describe());
        EXPECT_FALSE(build.dictionary.finish());
        EXPECT_FALSE(build.product.finish());
        EXPECT_EQ(before, snapshot());
        const auto resource = std::ranges::find(report.issues, "missing-resource", &AuthoringIssue::code);
        ASSERT_NE(resource, report.issues.end());
        EXPECT_EQ(resource->dictionaryId, 1000);
        ASSERT_EQ(resource->origins.size(), 1);
        EXPECT_EQ(resource->origins.front().context.filename, "birds.xml");
        EXPECT_EQ(resource->origins.front().context.record, "sparrow");
        ASSERT_TRUE(resource->origins.front().location);
        EXPECT_EQ(resource->origins.front().location->line, 2);

        require(build.dictionary.addAsset("missing.png", {}));
        require(build.dictionary.addHeadline({.headline = u"Sparrow", .entryId = entry.target()}));
        require(reserved.writeXml("<html><body>A second bird.</body></html>"));
        require(build.dictionary.addHeadline({.headline = u"Bird", .entryId = reserved.target()}));
        require(build.dictionary.setEntryOrder(std::array{uint32_t{7}}));
        EXPECT_FALSE(build.dictionary.finish());
        require(build.dictionary.setEntryOrder(std::array{uint32_t{9}, uint32_t{7}}));
        EXPECT_TRUE(require(build.dictionary.preflight()).ready());
        auto productReport = require(build.product.preflight());
        EXPECT_TRUE(productReport.valid());
        EXPECT_FALSE(productReport.ready());
        require(build.dictionary.finish());
        EXPECT_FALSE(build.dictionary.preflight());
        EXPECT_TRUE(require(build.product.preflight()).ready());
        require(build.product.finish());
        EXPECT_TRUE(DictionaryProduct::openAtPath(path));
    }

    TEST(Creation, FinalisationIoFailureRemainsTerminalAfterTheFilesystemIsRepaired)
    {
        Workspace workspace;
        const auto path = workspace.path / "Repair";
        BirbBuild build(path);
        build.addBirb();
        require(build.dictionary.finish());
        const auto staging = std::filesystem::directory_iterator(workspace.path)->path();
        const auto obstruction = staging / "Contents" / "Repair.json";
        std::filesystem::create_directory(obstruction);
        const auto failed = build.product.finish();
        ASSERT_FALSE(failed);
        std::filesystem::remove(obstruction);
        const auto retried = build.product.finish();
        ASSERT_FALSE(retried);
        EXPECT_EQ(failed.error(), retried.error());
        EXPECT_FALSE(build.product.preflight());
        EXPECT_FALSE(build.product.addAsset("repair.txt", bytes("too late")));
        EXPECT_FALSE(std::filesystem::exists(path));
    }
}
