//
// kiwakiwaaにより 2026/10/19 に作成されました。
//

#pragma once

#include "MKD/dictionary/navigation.hpp"
#include "binary/binary_view.hpp"
#include "unicode/utf8.hpp"
#include <array>

namespace MKD::detail::monokakido::navigation
{
    inline constexpr char rootPlist[] = "Root.plist";
    inline constexpr char rootIndex[] = "Top.idx";
    inline constexpr char directory[] = "index";
    inline constexpr char typeInfo[] = "IndexTypeInfo.plist";
    inline constexpr char generalController[] = "HMDicJapaneseIndexViewController";
    inline constexpr char kanwaController[] = "KLKanwaDicIndexRootViewController";
    inline constexpr char kanwaContents[] = "KLKanwaDicContentsViewController";
    inline constexpr char kanwaFormatter[] = "KLKanwaDicLabelFormatter";
    inline constexpr uint16_t sections = 1, gridHeaders = 2, listHeaders = 4,
                              columnBreaks = 8, compact = 16, hideSectionIndex = 32, parentHeadlines = 64;
    inline constexpr size_t headerSize = 16;
    inline constexpr size_t maximumDepth = 64;

    enum class ItemKind : uint8_t
    {
        Section,
        Entry,
        File,
        Reference,
        LabelledEntry
    };

    inline constexpr std::array<size_t, 5> strides{16, 8, 8, 8, 12};

    struct Header
    {
        uint8_t titleKind;
        ItemKind itemKind;
        uint16_t flags;
        uint32_t title;
        uint32_t count;
        uint32_t data;
        static Result<Header> read(BinaryView data);
    };

    Result<std::string> text(BinaryView data, size_t offset);
    Result<AppendixEntryLink> entry(BinaryView data, size_t offset);
    std::string encodeLabel(const NavigationLabel& label);
    NavigationLabel decodeLabel(std::string text);
    std::string plainLabel(const NavigationLabel& label);
}
