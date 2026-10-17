//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

namespace MKD::detail::metadata::language
{
    inline constexpr char english[] = "en";
    inline constexpr char japanese[] = "ja";
}

namespace MKD::detail::metadata::content
{
    inline constexpr char identifier[] = "DSContentIdentifier";
    inline constexpr char directory[] = "DSContentDirectory";
    inline constexpr char dictionaryId[] = "DSContentDictionaryID";
    inline constexpr char version[] = "DSContentVersion";
    inline constexpr char title[] = "DSContentTitle";
    inline constexpr char publisher[] = "DSContentPublisher";
    inline constexpr char edition[] = "DSContentEdition";
    inline constexpr char languages[] = "DSContentLanguages";
    inline constexpr char keyNormalization[] = "DSContentKeyNormalization";
    inline constexpr char hasHeadlineKeyIdentifier[] = "DSContentHasHeadlineKeyIdentifier";
    inline constexpr char headwordCount[] = "DSContentHeadwordCount";
    inline constexpr char audioCount[] = "DSContentAudioCount";
}

namespace MKD::detail::metadata::product
{
    inline constexpr char displayName[] = "DSProductDisplayName";
    inline constexpr char description[] = "DSProductDescription";
    inline constexpr char genre[] = "DSProductGenre";
    inline constexpr char title[] = "DSProductTitle";
    inline constexpr char identifier[] = "DSProductIdentifier";
    inline constexpr char category[] = "DSProductCategory";
    inline constexpr char version[] = "DSProductVersion";
    inline constexpr char searchClass[] = "DSProductSearchClass";
    inline constexpr char contents[] = "DSProductContents";
    inline constexpr char searchScopeNames[] = "DSProductSearchScopeNames";
    inline constexpr char fonts[] = "DSProductFonts";
    inline constexpr char verticalLayout[] = "DSProductVerticalLayout";
    inline constexpr char requiredDicKitVersion[] = "DSProductRequiredDicKitVersion";
}

namespace MKD::detail::metadata::package
{
    inline constexpr char identifier[] = "IAPProductIdentifier";
    inline constexpr char version[] = "ContentVersion";
    inline constexpr char platform[] = "PlatformID";
    inline constexpr char releaseType[] = "ReleaseType";
    inline constexpr char buildVersion[] = "BuildVersion";
}

namespace MKD::detail::metadata::appendix
{
    inline constexpr char name[] = "name";
    inline constexpr char children[] = "children";
    inline constexpr char contents[] = "contents";
}
