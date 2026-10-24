//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#pragma once

#include "MKD/resource/entry_id.hpp"
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace MKD
{
    /** Paths in menu targets are relative to the product's Contents directory. */
    struct AppendixList
    {
        std::string path;
        /** Shows one ungrouped list instead of headline sections. */
        bool singleSection = false;
        /** Uses each row's item ID when opening a subentry. */
        bool usingAnchorId = false;
        bool skipThe = false;
    };

    struct AppendixDocument
    {
        std::string path;
        std::optional<std::string> fragment;
    };

    /** A graphics.plist thumbnail gallery. Image paths are product-relative. */
    struct AppendixGallery
    {
        std::vector<std::string> images;
    };

    struct AppendixEntryLink
    {
        EntryId entryId;
        uint16_t dictionaryId;
    };

    /** Preserves custom presenters and unrecognised targets for inspection. Not accepted by the writer. */
    struct AppendixCustom
    {
        std::string presenter;
        std::optional<std::string> contents;
        std::optional<std::string> file;
        bool detail = false;
    };

    struct AppendixSection;

    struct AppendixMenu
    {
        std::vector<AppendixSection> sections;
    };

    using AppendixTarget =
        std::variant<AppendixList, AppendixDocument, AppendixEntryLink, AppendixMenu, AppendixGallery, AppendixCustom>;

    struct AppendixItem
    {
        /** Encoded headline markup is preserved in names and titles. */
        std::string name;
        AppendixTarget target;
        std::optional<std::string> title;
        std::optional<std::string> number;
        std::optional<std::string> description;
        std::optional<std::string> image;
        uint32_t indentation = 0;
    };

    struct AppendixSection
    {
        /** Empty names represent unlabelled sections. */
        std::string name;
        std::vector<AppendixItem> items;
    };
}
