//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#pragma once

#include "MKD/dictionary/product_writer.hpp"
#include "io/build_directory.hpp"
#include "authoring_diagnostics.hpp"
#include <memory>
#include <map>
#include <tuple>

namespace MKD::detail
{
    class DictionaryBuild;

    class ProductBuild : public std::enable_shared_from_this<ProductBuild>
    {
    public:
        static Result<std::shared_ptr<ProductBuild>> create(const std::filesystem::path& destination,
                                                            DictionaryMetadata metadata, CreationProfile profile);
        ProductBuild(std::unique_ptr<BuildDirectory> output, DictionaryMetadata metadata, std::string name,
                     CreationProfile profile);
        Result<std::shared_ptr<DictionaryBuild>> addDictionary(DictionaryContent content,
                                                               DictionaryWriteOptions options, SourceContext context);
        Result<void> writeAsset(std::string_view relative, std::span<const uint8_t> bytes) const;
        Result<void> setAppendices(AppendixMenu menu);
        Result<void> setNavigation(NavigationIndex index);
        Result<void> setInformation(ProductInformation information);
        Result<void> setIcon(std::span<const uint8_t> image);
        Result<void> finish();
        Result<void> validateTarget(const AppendixEntryLink& target) const;
        void referenceAppendix(AppendixEntryLink target, const DictionaryContent& dictionary, AuthoringOrigin origin);
        [[nodiscard]] bool hasFont(std::string_view filename) const;
        void registerFont(std::string filename) { metadata_.fonts.push_back(std::move(filename)); }
        void configureNavigation(bool kanwa);
        [[nodiscard]] const std::filesystem::path& path() const { return output_->path(); }
        [[nodiscard]] Result<void> check() const { return output_->check(); }
        Result<void> fail(std::string error) const { return output_->fail(std::move(error)); }
        [[nodiscard]] const DictionaryMetadata& metadata() const { return metadata_; }
        [[nodiscard]] const std::string& name() const { return name_; }
        [[nodiscard]] CreationProfile profile() const { return profile_; }
        [[nodiscard]] const ProductInformation& information() const { return information_; }
        [[nodiscard]] const DictionaryContent& firstContent() const;

    private:
        Result<void> writeAppendices() const;
        struct AppendixReference
        {
            AppendixEntryLink target;
            std::optional<uint16_t> dictionaryId;
            std::string dictionary;
            DependencyOrigins origins;
        };
        std::unique_ptr<BuildDirectory> output_;
        DictionaryMetadata metadata_;
        std::string name_;
        CreationProfile profile_;
        ProductInformation information_;
        std::optional<AppendixMenu> appendices_;
        std::optional<NavigationIndex> navigation_;
        std::map<std::tuple<uint16_t, uint16_t, EntryId>, AppendixReference> appendixReferences_;
        std::vector<std::shared_ptr<DictionaryBuild>> dictionaries_;
    };
}
