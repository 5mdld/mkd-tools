//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

namespace MKD::detail::metadata::language
{
    inline constexpr char english[] = "en";
    inline constexpr char japanese[] = "ja";
    inline constexpr char french[] = "fr";
    inline constexpr char italian[] = "it";
    inline constexpr char german[] = "de";
    inline constexpr char spanish[] = "es";
    inline constexpr char portuguese[] = "pt";
}

namespace MKD::detail::metadata::content
{
    inline constexpr char identifier[] = "DSContentIdentifier";
    inline constexpr char type[] = "DSContentType";
    inline constexpr char directory[] = "DSContentDirectory";
    inline constexpr char dictionaryId[] = "DSContentDictionaryID";
    inline constexpr char version[] = "DSContentVersion";
    inline constexpr char title[] = "DSContentTitle";
    inline constexpr char publisher[] = "DSContentPublisher";
    inline constexpr char edition[] = "DSContentEdition";
    inline constexpr char languages[] = "DSContentLanguages";
    inline constexpr char keyNormalization[] = "DSContentKeyNormalization";
    inline constexpr char hasHeadlineKeyIdentifier[] = "DSContentHasHeadlineKeyIdentifier";
    inline constexpr char fullRubyStylesheet[] = "DSContentFullRubyCSS";
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
    inline constexpr char delegateClass[] = "DSProductDelegateClass";
    inline constexpr char searchClass[] = "DSProductSearchClass";
    inline constexpr char contents[] = "DSProductContents";
    inline constexpr char searchScopeNames[] = "DSProductSearchScopeNames";
    inline constexpr char fonts[] = "DSProductFonts";
    inline constexpr char verticalLayout[] = "DSProductVerticalLayout";
    inline constexpr char appendixViewController[] = "DSProductAppendixViewController";
    inline constexpr char contentsViewController[] = "DSProductContentsViewController";
    inline constexpr char labelFormatter[] = "DSProductLabelFormatter";
    inline constexpr char appendixFirst[] = "DSProductAppendixFirst";
    inline constexpr char requiredDicKitVersion[] = "DSProductRequiredDicKitVersion";
}

namespace MKD::detail::metadata::package
{
    inline constexpr char identifier[] = "IAPProductIdentifier";
    inline constexpr char version[] = "ContentVersion";
    inline constexpr char platform[] = "PlatformID";
    inline constexpr char releaseType[] = "ReleaseType";
    inline constexpr char buildVersion[] = "BuildVersion";
    inline constexpr char mkdPackageVersion[] = "MKDPackageVersion";
}

namespace MKD::detail::metadata::appendix
{
    inline constexpr char name[] = "name";
    inline constexpr char children[] = "children";
    inline constexpr char contents[] = "contents";
    inline constexpr char title[] = "title";
    inline constexpr char number[] = "num";
    inline constexpr char description[] = "description";
    inline constexpr char image[] = "image";
    inline constexpr char indentation[] = "level";
    inline constexpr char entry[] = "entry";
    inline constexpr char presenter[] = "presenter";
    inline constexpr char file[] = "file";
    inline constexpr char detail[] = "detail";
    inline constexpr char singleSection[] = "single-section";
    inline constexpr char usingAnchorId[] = "using-anchor-id";
    inline constexpr char skipThe[] = "skipping-the";
}
