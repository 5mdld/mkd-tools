//
// kiwakiwaaにより 2026/02/24 に作成されました。
//

#pragma once

#include "MKD/dictionary/metadata.hpp"
#include "MKD/dictionary/appendix_menu.hpp"
#include "MKD/dictionary/navigation.hpp"
#include "MKD/dictionary/paths.hpp"
#include "MKD/dictionary/dictionary.hpp"

namespace MKD
{
    class DictionaryProduct
    {
    public:
        DictionaryProduct(DictionaryProduct&&) noexcept = default;
        DictionaryProduct& operator=(DictionaryProduct&&) noexcept = default;
        DictionaryProduct(const DictionaryProduct&) = delete;
        DictionaryProduct& operator=(const DictionaryProduct&) = delete;

        static Result<DictionaryProduct> openAtPath(const fs::path& path);

        // Lightweight open: loads metadata and paths only.
        // Dictionaries are created with empty resources.
        static Result<DictionaryProduct> loadMetadata(const fs::path& path);

        [[nodiscard]] const DictionaryMetadata& metadata() const noexcept { return metadata_; }
        [[nodiscard]] const DictionaryPaths& paths() const noexcept { return paths_; }
        [[nodiscard]] const std::vector<Dictionary>& dictionaries() const noexcept { return dictionaries_; }
        [[nodiscard]] std::vector<Dictionary>& dictionaries() noexcept { return dictionaries_; }
        [[nodiscard]] const AppendixMenu& appendices() const noexcept { return appendices_; }
        /** Maps a loose product-relative asset on demand. */
        [[nodiscard]] Result<RetainedSpan> resource(std::string_view relativePath) const;
        [[nodiscard]] Result<AppendixEntryList> openAppendixList(const AppendixList& list) const;
        /** Maps document bytes on demand. The returned span retains its mapping independently of this product. */
        [[nodiscard]] Result<RetainedSpan> openAppendixDocument(const AppendixDocument& document) const;
        /** Opens the root navigation index on demand. Absence means the product uses ordinary appendices. */
        [[nodiscard]] Result<std::optional<NavigationView>> navigation() const;
        [[nodiscard]] std::string displayTitle() const;
        [[nodiscard]] std::optional<fs::path> iconPath() const;

        // Find a specific content dictionary by its content identifier
        [[nodiscard]] const Dictionary* findContent(std::string_view contentId) const;
        [[nodiscard]] Dictionary* findContent(std::string_view contentId);
        [[nodiscard]] const Dictionary* findContent(uint16_t dictionaryId) const;
        [[nodiscard]] Dictionary* findContent(uint16_t dictionaryId);

    private:
        DictionaryProduct(DictionaryMetadata metadata, DictionaryPaths paths,
                          std::vector<Dictionary> dictionaries, AppendixMenu appendices);

        AppendixMenu appendices_;
        DictionaryMetadata metadata_;
        DictionaryPaths paths_;
        std::vector<Dictionary> dictionaries_;
    };
}
