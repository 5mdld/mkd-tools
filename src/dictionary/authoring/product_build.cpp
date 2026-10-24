//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#include "product_build.hpp"
#include "dictionary_build.hpp"
#include "product_information.hpp"
#include "product_icon.hpp"
#include "dictionary/authoring/metadata_preparation.hpp"
#include "dictionary/package/metadata_writer.hpp"
#include "dictionary/package/package_format.hpp"
#include "dictionary/search/search_definitions.hpp"
#include "dictionary/search/search_definitions.hpp"
#include "dictionary/authoring/navigation_writer.hpp"
#include "dictionary/navigation/index_format.hpp"
#include "dictionary/authoring/appendix_writer.hpp"
#include "io/write_validation.hpp"
#include "output/directory_output.hpp"
#include <algorithm>

namespace MKD::detail
{
    ProductBuild::ProductBuild(std::unique_ptr<BuildDirectory> output, DictionaryMetadata metadata, std::string name,
                               const CreationProfile profile)
        : output_(std::move(output)), metadata_(std::move(metadata)), name_(std::move(name)), profile_(profile)
    {
    }

    const DictionaryContent& ProductBuild::firstContent() const
    {
        return dictionaries_.front()->metadata();
    }

    bool ProductBuild::hasFont(std::string_view filename) const
    {
        return std::ranges::find(metadata_.fonts, filename) != metadata_.fonts.end();
    }

    Result<void> ProductBuild::validateTarget(const AppendixEntryLink& target) const
    {
        const auto dictionary = std::ranges::find_if(dictionaries_, [&](const auto& item)
        {
            return item->metadata().dictionaryId == target.dictionaryId;
        });

        if (dictionary == dictionaries_.end())
            return std::unexpected("Entry targets an unknown dictionary");

        return (*dictionary)->validateTarget(target.entryId);
    }

    void ProductBuild::configureNavigation(bool kanwa)
    {
        metadata_.appendixViewController =
            kanwa ? native::navigation::kanwaController : native::navigation::generalController;
        metadata_.appendixFirst = true;
    }

    Result<void> ProductBuild::setAppendices(const AppendixMenu value)
    {
        if (auto state = output_->check(); !state)
            return state;

        appendices_ = std::move(value);
        return {};
    }

    Result<void> ProductBuild::setNavigation(const NavigationIndex value)
    {
        if (auto state = output_->check(); !state)
            return state;

        navigation_ = std::move(value);
        return {};
    }

    Result<void> ProductBuild::setInformation(const ProductInformation value)
    {
        if (auto state = output_->check(); !state)
            return state;

        if (auto valid = ProductInformationWriter::validate(value); !valid)
            return output_->fail(valid.error());

        information_ = std::move(value);
        return {};
    }

    Result<void> ProductBuild::setIcon(std::span<const uint8_t> bytes)
    {
        if (auto state = output_->check(); !state)
            return state;

        if (auto written = ProductIcon::write(*this, bytes); !written)
            return output_->fail(written.error());

        return {};
    }

    Result<std::shared_ptr<ProductBuild>> ProductBuild::create(const std::filesystem::path& destination,
                                                               DictionaryMetadata metadata, CreationProfile profile)
    {
        auto prepared = metadata_preparation::prepareProduct(std::move(metadata), destination, profile);
        if (!prepared)
            return std::unexpected(prepared.error());

        metadata = std::move(*prepared);

        auto output = BuildDirectory::create(destination);
        if (!output)
            return std::unexpected(output.error());

        std::error_code error;
        std::filesystem::create_directory((*output)->path() / native::paths::productContents, error);
        if (error)
            return std::unexpected(error.message());

        const auto name = destination.stem().generic_u8string();
        auto product = std::make_shared<ProductBuild>(
            std::move(*output), std::move(metadata),
            std::string(reinterpret_cast<const char*>(name.data()), name.size()), profile);

        return product;
    }

    Result<std::shared_ptr<DictionaryBuild>> ProductBuild::addDictionary(DictionaryContent content,
                                                                         DictionaryWriteOptions options)
    {
        if (auto state = output_->check(); !state)
            return std::unexpected(state.error());

        auto prepared = metadata_preparation::prepareContent(std::move(content));
        if (!prepared)
        {
            output_->fail(prepared.error());
            return std::unexpected(prepared.error());
        }

        content = std::move(*prepared);
        for (const auto& existing : dictionaries_)
        {
            if (existing->metadata().identifier == content.identifier ||
                existing->metadata().directory == content.directory ||
                existing->metadata().dictionaryId == content.dictionaryId)
            {
                output_->fail("Duplicate content identifier, directory or numeric dictionary ID");
                return std::unexpected("Duplicate content identifier, directory or numeric dictionary ID");
            }
        }

        auto dictionary = std::make_shared<DictionaryBuild>(shared_from_this(), std::move(content), options);
        std::error_code error;
        if (!std::filesystem::create_directory(dictionary->root(), error))
        {
            const auto message =
                "Cannot create content directory: " + (error ? error.message() : "path already exists");
            output_->fail(message);
            return std::unexpected(message);
        }

        dictionaries_.push_back(dictionary);
        return dictionary;
    }

    Result<void> ProductBuild::writeAsset(std::string_view relative, std::span<const uint8_t> bytes) const
    {
        if (auto state = output_->check(); !state)
            return state;

        if (auto valid = WriteValidation::relativePath(relative); !valid)
            return output_->fail(valid.error());

        if (relative == name_ + ".json" || relative == native::paths::appendixInfo ||
            relative == native::paths::moreInfo)
            return output_->fail("Asset path is reserved for generated product metadata");

        const auto path = WriteValidation::fromUtf8(relative);
        if (*path.begin() == native::paths::appendixMenus)
            return output_->fail("Asset path is reserved for generated appendix menus");

        for (const auto& dictionary : dictionaries_)
            if (*path.begin() == WriteValidation::fromUtf8(dictionary->metadata().directory))
                return output_->fail("Use the dictionary writer for content assets");

        DirectoryOutput directory(output_->path() / native::paths::productContents, false, false);
        if (auto prepared = directory.prepare(); !prepared)
            return output_->fail(prepared.error());

        auto written = directory.writeFile(path, [bytes](OutputFile& file) { return file.write(bytes); });
        if (!written)
            return output_->fail(written.error());

        if (written->skipped)
            return output_->fail("Duplicate product asset");

        return {};
    }

    Result<void> ProductBuild::writeAppendices() const
    {
        for (const auto& reference : appendixReferences_)
            if (auto valid = validateTarget(reference); !valid)
                return valid;
        if (appendices_)
            return AppendixWriter::write(*this, *appendices_);
        AppendixMenu menu;
        for (const auto& dictionary : dictionaries_)
        {
            if (dictionary->appendices().empty())
                continue;
            AppendixSection section{
                dictionary->metadata().title
                    ? dictionary->metadata().title->resolve()
                    : dictionary->metadata().directory,
                {}
            };
            section.items = dictionary->appendices();
            menu.sections.push_back(std::move(section));
        }
        return menu.sections.empty() ? Result<void>{} : AppendixWriter::write(*this, menu);
    }

    Result<void> ProductBuild::finish()
    {
        if (auto state = output_->check(); !state)
            return state;

        if (dictionaries_.empty())
            return output_->fail("Product has no dictionaries");

        std::set<SearchScope> scopes;
        for (const auto& dictionary : dictionaries_)
        {
            if (!dictionary->finished())
                return output_->fail("Finish every dictionary before finishing the product");

            metadata_.contents.push_back(dictionary->metadata());
            scopes.insert(dictionary->scopes().begin(), dictionary->scopes().end());
        }

        if (profile_ == CreationProfile::Kanwa)
        {
            for (const auto& scope : native::search::kanwaScopes)
                if (scopes.contains(scope.scope))
                    metadata_.searchScopeNames.emplace_back(scope.name);
        }
        else
            for (auto scope : scopes)
                metadata_.searchScopeNames.emplace_back(search::scopeDefinition(scope, profile_)->name);

        std::ranges::sort(metadata_.fonts);
        if (auto result = ProductInformationWriter::write(*this); !result)
            return output_->fail(result.error());

        if (auto result = writeAppendices(); !result)
            return output_->fail(result.error());

        if (navigation_)
            if (auto result = NavigationWriter::write(*this, *navigation_); !result)
                return output_->fail(result.error());

        if (auto result = metadata_writer::writeProduct(output_->path(), name_, metadata_); !result)
            return output_->fail(result.error());

        return output_->publish();
    }
}
