//
// kiwakiwaaにより 2026/10/19 に作成されました。
//

#include "support/test_dictionary.hpp"
#include "binary/binary_buffer.hpp"

namespace MKD::test
{
    class NavigationReferenceTest : public ::testing::Test
    {
    protected:
        void writeTree(const uint32_t selectedChild)
        {
            detail::BinaryBuffer data;
            data.writeLittle(uint32_t{1});

            // Two selectable child headers follow the root. The first contains
            // a compound target, the second aliases a subtree in this same file.
            for (const auto [kind, count, offset] : std::array{
                     std::array{0u, 2u, 16u},
                     std::array{1u, 1u, 32u},
                     std::array{3u, 1u, 24u}
                 })
            {
                data.writeByte(0);
                data.writeByte(static_cast<uint8_t>(kind));
                data.writeLittle(uint16_t{0});
                data.writeLittle(uint32_t{0});
                data.writeLittle(count);
                data.writeLittle(offset);
            }

            data.writeLittle(uint32_t{7});
            data.writeLittle(uint16_t{0x8001});
            data.writeLittle(uint16_t{1000});

            // Reference offsets are relative to its child header at byte 36.
            // The filename starts at byte 68 and the section path at byte 88.
            data.writeLittle(uint32_t{32});
            data.writeLittle(uint32_t{52});
            data.writeLittle(uint32_t{16});
            for (const auto unit : std::u16string_view(u"birb.idx"))
                data.writeLittle(static_cast<uint16_t>(unit));

            data.writeLittle(uint32_t{1});
            data.writeLittle(selectedChild);
            writeFile(workspace.path / "birb.idx", data.data());
        }

        Workspace workspace;
    };

    TEST_F(NavigationReferenceTest, SameFileSubtreeAliasesRetainTheirMapping)
    {
        writeTree(0);
        auto subtree = [&]
        {
            auto root = require(NavigationView::open(workspace.path, {"birb.idx", {}}));
            return std::get<NavigationView>(require(root.itemAt(0, 1)).target);
        }();

        const auto entry = std::get<AppendixEntryLink>(require(subtree.itemAt(0, 0)).target);
        EXPECT_EQ(entry.entryId, (EntryId{7, 0x8001}));
        EXPECT_EQ(entry.dictionaryId, 1000);

        auto selected = require(NavigationView::open(workspace.path, {"birb.idx", {0}}));
        EXPECT_EQ(std::get<AppendixEntryLink>(require(selected.itemAt(0, 0)).target).entryId, entry.entryId);
        EXPECT_FALSE(NavigationView::open(workspace.path, {"birb.idx", {2}}));
    }

    TEST_F(NavigationReferenceTest, CyclicAliasesDoNotPreventReadingOtherBranches)
    {
        writeTree(1);
        auto root = require(NavigationView::open(workspace.path, {"birb.idx", {}}));
        auto cycle = root.itemAt(0, 1);
        ASSERT_FALSE(cycle);
        EXPECT_NE(cycle.error().find("Cyclic"), std::string::npos);

        auto sibling = std::get<NavigationView>(require(root.itemAt(0, 0)).target);
        EXPECT_EQ(std::get<AppendixEntryLink>(require(sibling.itemAt(0, 0)).target).entryId, (EntryId{7, 0x8001}));
        EXPECT_FALSE(NavigationView::open(workspace.path, {"birb.idx", {1}}));
    }

    TEST(Navigation, GeneralGridsAndAppendicesRemainReachable)
    {
        auto product = require(DictionaryProduct::openAtPath(dictionaries() / "Birbs"));
        auto root = require(product.navigation());
        ASSERT_TRUE(root);
        const auto birbs = require(root->itemAt(0, 0));
        EXPECT_EQ(std::get<std::string>(birbs.label), "小鳥");
        auto children = std::get<NavigationView>(birbs.target);
        EXPECT_EQ(require(children.itemCount(0)), 2);
        const auto target = std::get<AppendixEntryLink>(require(children.itemAt(0, 0)).target);
        EXPECT_EQ(target.entryId, (EntryId{1, 0}));
        EXPECT_EQ(target.dictionaryId, 1000);
        EXPECT_EQ(std::get<std::string>(require(root->itemAt(0, 2)).label), "付録");
    }

    TEST(Navigation, KanwaChildrenRetainTheirMapping)
    {
        auto root = []
        {
            auto product = require(DictionaryProduct::openAtPath(dictionaries() / "BirbKanji"));
            return *require(product.navigation());
        }();

        EXPECT_EQ(root.kindCode(), 1);
        auto radicals = std::get<NavigationView>(require(root.itemAt(0, 0)).target);
        EXPECT_EQ(radicals.kindCode(), 2);
        auto radical = require(radicals.itemAt(0, 0));
        EXPECT_EQ(std::get<RadicalLabel>(radical.label).reading, "とり");
        auto members = std::get<NavigationView>(radical.target);
        EXPECT_EQ(require(members.sectionTitle(0)), "部首 ＋ 0画");
        EXPECT_EQ(std::get<AppendixEntryLink>(require(members.itemAt(0, 0)).target).entryId, (EntryId{1, 0}));
        EXPECT_FALSE(members.itemAt(0, 1));
    }

    TEST(Navigation, MalformedRecordsAndEscapingReferencesAreRejected)
    {
        Workspace workspace;
        detail::BinaryBuffer data;
        data.writeLittle(uint32_t{2});
        data.writeByte(0);
        data.writeByte(1);
        data.writeLittle(uint16_t{0});
        data.writeLittle(uint32_t{0});
        data.writeLittle(UINT32_MAX);
        data.writeLittle(uint32_t{16});
        writeFile(workspace.path / "bad.idx", data.data());
        EXPECT_FALSE(NavigationView::open(workspace.path, {"bad.idx", {}}));
        EXPECT_FALSE(NavigationView::open(workspace.path, {"../outside.idx", {}}));
    }

    TEST(Navigation, MissingTargetsPreventPublication)
    {
        Workspace workspace;
        BirbBuild build(workspace.path / "Birbs");
        build.addBirb();
        require(build.product.setNavigation({
            .sections = {{"", {{std::string("Lost birb"), AppendixEntryLink{{99, 0}, 0}}}}}
        }));
        require(build.dictionary.finish());
        EXPECT_FALSE(build.product.finish());
        EXPECT_FALSE(std::filesystem::exists(workspace.path / "Birbs"));
    }
}
