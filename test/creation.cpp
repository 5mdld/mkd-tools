//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "support/test_dictionary.hpp"
#include <array>

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
        require(entry.writeXml("<html><body><img src=\"graphics/saberlion.png\"/></body></html>"));
        require(missing.dictionary.addHeadline({.headline = u"獅子", .entryId = entry.target()}));
        require(missing.dictionary.addSearchKey(SearchScope::Headword, "saberlion", entry.target()));
        EXPECT_FALSE(missing.dictionary.finish());
        EXPECT_FALSE(std::filesystem::exists(workspace.path / "MissingBirb"));
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
}
