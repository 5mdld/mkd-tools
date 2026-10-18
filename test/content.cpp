//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "support/test_dictionary.hpp"
#include <unordered_map>

namespace MKD::test
{
    using ContentTest = DictionaryTest;

    TEST(InlineContent, RubyGaijiAndLinksShareXmlAndHeadlineRepresentations)
    {
        InlineText label;
        label.append(Ruby{"鶫", "つぐみ"});
        label.append(" & ");
        label.append(Gaiji{{"gaiji/tsugumi.svg"}, "鶫"});
        label.append(EntryLink{{3, 0}, "セイバーライオン"});
        const auto headline = require(label.toHeadline());
        EXPECT_EQ(require(InlineText::fromHeadline(headline)).toHeadline().value(), headline);
        EXPECT_EQ(require(label.plainText()), "鶫 & 鶫セイバーライオン");
        const auto xml = require(label.toXhtml());
        EXPECT_NE(xml.find("<ruby>"), std::string::npos);
        EXPECT_NE(xml.find("&amp;"), std::string::npos);
        EXPECT_NE(xml.find("href=\"00003-0000\""), std::string::npos);
        EXPECT_NE(xml.find("gaiji/tsugumi.svg"), std::string::npos);
    }

    TEST(InlineContent, PreservesUnrecognisedNativeMarkupWithoutInventingXml)
    {
        const auto text = require(InlineText::fromHeadline(u"{B:がおー}"));
        EXPECT_EQ(require(text.toHeadline()), u"{B:がおー}");
        EXPECT_FALSE(text.toXhtml());
        EXPECT_FALSE(text.plainText());
        EXPECT_FALSE(InlineText::fromHeadline(u"{RB:broken"));
        InlineText unsafe;
        unsafe.append(Gaiji{{"../saberlion.png"}, "獅子"});
        EXPECT_FALSE(unsafe.toXhtml());
    }

    TEST_F(ContentTest, ResolvesLooseGaijiAndRegisteredStylesheets)
    {
        const auto gaiji = require(birbs().resource("gaiji/tsugumi.svg"));
        EXPECT_EQ(readFile(assets() / "gaiji.svg"), std::vector(gaiji.begin(), gaiji.end()));
        ASSERT_TRUE(birbs().stylesheet());
        ASSERT_TRUE(birbs().nightmodeStylesheet());
        EXPECT_EQ(birbs().presentation().fullRubyStylesheet, "birbs-ruby.css");
        EXPECT_NE(require(birbs().resource("birbs.css")).utf8().value().find("BirbMincho"), std::string::npos);
    }

    TEST(EntryIds, CanonicalTargetsWorkAsHashKeys)
    {
        const EntryId target{7, 0x8001};
        EXPECT_EQ(target.toString(), "00007-8001");
        std::unordered_map<EntryId, std::string> names{{target, "雀躍"}};
        EXPECT_EQ(names.at(target), "雀躍");
    }
}
