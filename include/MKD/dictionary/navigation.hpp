//
// kiwakiwaaにより 2026/10/19 に作成されました。
//

#pragma once

#include "MKD/dictionary/appendix_menu.hpp"
#include "MKD/result.hpp"
#include <filesystem>
#include <memory>

namespace MKD
{
    enum class NavigationKind : uint32_t
    {
        Generic = 0,
        Root = 1,
        Radicals = 2,
        Readings = 3,
        TotalStrokes = 4,
        Classifications = 5,
        Names = 6,
        Images = 7,
        Etymology = 8
    };

    struct RadicalLabel
    {
        std::string character;
        std::string variants;
        std::string reading;
    };

    using NavigationLabel = std::variant<std::string, RadicalLabel, AppendixEntryLink>;

    /** References an existing index. Paths are relative to the product's Contents directory. */
    struct NavigationReference
    {
        std::string path;
        std::vector<uint32_t> sectionPath;
    };

    struct NavigationSection;

    struct NavigationIndex
    {
        NavigationKind kind = NavigationKind::Generic;
        std::vector<NavigationSection> sections;
        bool compact = false;
        bool sectionIndex = true;
        bool parentHeadlines = false;
        /** General dictionary grids use these dimensions, or choose them automatically when zero. */
        uint32_t rows = 0;
        uint32_t columns = 0;
        bool vertical = true;
        bool shortenHeadlines = false;
    };

    using NavigationTarget = std::variant<std::monostate, AppendixEntryLink, AppendixDocument,
                                          AppendixList, NavigationIndex, NavigationReference>;

    struct NavigationItem
    {
        /** Empty labels use the target entry's headline. Empty targets are grid spacers. */
        NavigationLabel label = std::string{};
        NavigationTarget target;
    };

    struct NavigationSection
    {
        std::string title;
        std::vector<NavigationItem> items;
    };

    struct NavigationItemView;

    /** A lazy navigation node. Copies and child views retain their backing data independently. */
    class NavigationView
    {
    public:
        static Result<NavigationView> open(const std::filesystem::path& contents,
                                           const NavigationReference& reference);
        /** Kind code stored in a binary navigation index. Property-list menus report zero. */
        [[nodiscard]] uint32_t kindCode() const noexcept;
        [[nodiscard]] size_t sectionCount() const noexcept;
        [[nodiscard]] Result<std::string> sectionTitle(size_t section) const;
        [[nodiscard]] Result<size_t> itemCount(size_t section) const;
        [[nodiscard]] Result<NavigationItemView> itemAt(size_t section, size_t index) const;

    private:
        struct Impl;
        std::shared_ptr<const Impl> impl_;

        explicit NavigationView(std::shared_ptr<const Impl> impl) : impl_(std::move(impl))
        {
        }
    };

    using NavigationViewTarget = std::variant<std::monostate, AppendixEntryLink, AppendixDocument, NavigationView>;

    struct NavigationItemView
    {
        NavigationLabel label;
        NavigationViewTarget target;
    };
}
