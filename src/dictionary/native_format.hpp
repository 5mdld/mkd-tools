//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/dictionary/detail/metadata_keys.hpp"
#include <array>
#include <cstdint>
#include <string_view>

namespace MKD::detail::native::entry
{
    struct TargetRange
    {
        uint16_t first;
        uint16_t last;
    };

    // In EntryTargetKind order. Leave headline reference and auxiliary identifier ranges unallocated.
    inline constexpr std::array targetRanges{
        TargetRange{0x4001, 0x4FFF}, TargetRange{0x8001, 0x8FFF},
        TargetRange{0x5001, 0x5FFF}, TargetRange{0xF001, 0xFFFF},
        TargetRange{0xC001, 0xCFFF}
    };
}

namespace MKD::detail::native::paths
{
    inline constexpr char productContents[] = "Contents";
    inline constexpr char productInfo[] = "ContentInfo.plist";
    inline constexpr char contentInfo[] = "DSContentInfo.plist";
    inline constexpr char appendixInfo[] = "Appendix.plist";
    inline constexpr char appendixMenus[] = "appendix-menus";
    inline constexpr char graphicsInfo[] = "graphics.plist";
    inline constexpr char moreInfo[] = "More.plist";
    inline constexpr char informationPage[] = "about.html";
    inline constexpr char icons[] = "icons";
    inline constexpr char contents[] = "contents";
    inline constexpr char keys[] = "key";
    inline constexpr char secondaryKeys[] = "key2";
    inline constexpr char headlines[] = "headline";
    inline constexpr char audio[] = "audio";
    inline constexpr char graphics[] = "graphics";
    inline constexpr char legacyGraphics[] = "img";
    inline constexpr char supplementaryHeadwords[] = "headword-plus.keystore";
    inline constexpr char fonts[] = "fonts";
    inline constexpr char entryOrder[] = "contents-order.dat";
}

namespace MKD::detail::native::defaults
{
    inline constexpr char productIdentifierPrefix[] = "jp.monokakido.Dictionaries.";
    inline constexpr char productVersion[] = "1.0";
    inline constexpr char contentVersion[] = "1";
    inline constexpr char platform[] = "macOS";
    inline constexpr char releaseType[] = "local";
    inline constexpr char buildVersion[] = "1";
}

namespace MKD::detail::native::paths
{
    inline constexpr std::array headlineFiles{
        "headline.headlinestore", "headline2.headlinestore", "short-headline.headlinestore"
    };
}

namespace MKD::detail::native::modernEntries
{
    inline constexpr char8_t filenameSuffix[] = u8"-GendaiEntryRange.plist";
    inline constexpr char minimumKey[] = "MinimumEntryID";
    inline constexpr char maximumKey[] = "MaximumEntryID";
}

namespace MKD::detail::native::search
{
    inline constexpr std::array<uint16_t, 2> classicalComplexDictionaries{75, 82};
    inline constexpr uint16_t normalizedHeadlineOffset = 2048;
    inline constexpr uint16_t modernHeadlineOffset = 2304;
    inline constexpr size_t modernHeadlinePrefixLength = 3;
}

namespace MKD::detail::native::contentTypes
{
    inline constexpr char dictionary[] = "Dictionary";
}

namespace MKD::detail::native::information
{
    inline constexpr char items[] = "items";
    inline constexpr char name[] = "name";
    inline constexpr char contents[] = "contents";
    inline constexpr char copyright[] = "Copyright";
    inline constexpr char settings[] = "$settings$";
}

namespace MKD::detail::native::icons
{
    struct Variant
    {
        std::string_view suffix;
        int pixels;
    };

    inline constexpr std::array variants{
        Variant{"-20.png", 20}, Variant{"-20@2x.png", 40}, Variant{"-20@3x.png", 60},
        Variant{"-29.png", 29}, Variant{"-29@2x.png", 58}, Variant{"-29@3x.png", 87},
        Variant{"-60.png", 60}, Variant{"-60@2x.png", 120}, Variant{"-60@3x.png", 180},
        Variant{"-76.png", 76}, Variant{"-76@2x.png", 152}, Variant{"-76@3x.png", 228},
        Variant{"-83.5@2x.png", 167},
    };
}
