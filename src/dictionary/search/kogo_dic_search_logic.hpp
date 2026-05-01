//
// KLKogoDicSearchLogic reconstruction.
//

#pragma once

#include "dic_complex_search_logic.hpp"

#include <optional>

namespace MKD::detail::search
{
    class KogoDicSearchLogic final : public DicComplexSearchLogic
    {
    public:
        using DicComplexSearchLogic::DicComplexSearchLogic;

        [[nodiscard]] Result<SearchResult> search(std::string_view query, const SearchOptions& options) override;

    protected:
        [[nodiscard]] bool usesComplexSearchForScope(SearchScope scope, const Dictionary& dictionary) const override;
        [[nodiscard]] uint32_t normalizedHeadlineAnchorOffset() const noexcept override;
        [[nodiscard]] std::vector<std::string> normalizedKeysForQuery(std::string_view query) const override;

    private:
        [[nodiscard]] static std::string_view gendaiHeadlineKeyText(std::string_view headline) noexcept;

        struct EntryIdRange
        {
            uint32_t minimumEntryId = 0;
            uint32_t maximumEntryId = 0;

            [[nodiscard]] bool contains(const EntryId& entryId) const noexcept;
        };

        [[nodiscard]] static std::optional<EntryIdRange> gendaiEntryRangeForContent(
            std::string_view contentIdentifier) noexcept;

        void sortGendaiHeadwordResults(SearchResult& result, std::string_view query) const;
    };
}
