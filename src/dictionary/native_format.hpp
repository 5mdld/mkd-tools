//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/dictionary/detail/metadata_keys.hpp"
#include <array>
#include <cstdint>

namespace MKD::detail::native::paths
{
    inline constexpr char productContents[] = "Contents";
    inline constexpr char productInfo[] = "ContentInfo.plist";
    inline constexpr char contentInfo[] = "DSContentInfo.plist";
    inline constexpr char appendixInfo[] = "Appendix.plist";
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
