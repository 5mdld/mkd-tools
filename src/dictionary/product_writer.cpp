//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "MKD/dictionary/product_writer.hpp"
#include "native_format.hpp"
#include "native_search.hpp"
#include "creation_metadata.hpp"
#include "creation_session.hpp"
#include "io/write_validation.hpp"
#include "product_information.hpp"
#include "product_icon.hpp"

#include <algorithm>
#include <set>

namespace MKD
{
    struct ProductWriter::Impl
    {
        std::shared_ptr<detail::ProductBuild> product;

        explicit Impl(std::shared_ptr<detail::ProductBuild> product) : product(std::move(product))
        {
        }
    };

    ProductWriter::ProductWriter(std::unique_ptr<Impl> impl) : impl_(std::move(impl))
    {
    }

    ProductWriter::~ProductWriter() = default;
    ProductWriter::ProductWriter(ProductWriter&&) noexcept = default;
    ProductWriter& ProductWriter::operator=(ProductWriter&&) noexcept = default;

    Result<ProductWriter> ProductWriter::create(const std::filesystem::path& destination, DictionaryMetadata metadata,
                                                CreationProfile profile)
    {
        if (auto valid = detail::CreationMetadata::validateProduct(metadata, destination, profile); !valid)
            return std::unexpected(valid.error());

        auto output = detail::BuildDirectory::create(destination);
        if (!output)
            return std::unexpected(output.error());

        std::error_code error;
        std::filesystem::create_directory((*output)->path() / detail::native::paths::productContents, error);
        if (error)
            return std::unexpected(error.message());

        const auto name = destination.stem().generic_u8string();
        auto product = std::make_shared<detail::ProductBuild>(std::move(*output), std::move(metadata),
                                                              std::string(
                                                                  reinterpret_cast<const char*>(name.data()),
                                                                  name.size()));

        product->profile = profile;
        return ProductWriter(std::make_unique<Impl>(std::move(product)));
    }

    Result<DictionaryWriter> ProductWriter::addDictionary(DictionaryContent content,
                                                          DictionaryWriteOptions options) const
    {
        auto& product = *impl_->product;
        if (auto state = product.output->check(); !state)
            return std::unexpected(state.error());

        if (auto valid = detail::CreationMetadata::validateContent(content); !valid)
        {
            product.output->fail(valid.error());
            return std::unexpected(valid.error());
        }

        for (const auto& existing : product.dictionaries)
        {
            if (existing->metadata.identifier == content.identifier || existing->metadata.directory == content.directory
                ||
                existing->metadata.dictionaryId == content.dictionaryId)
            {
                product.output->fail("Duplicate content identifier, directory or numeric dictionary ID");
                return std::unexpected("Duplicate content identifier, directory or numeric dictionary ID");
            }
        }

        auto dictionary = std::make_shared<detail::DictionaryBuild>(impl_->product, std::move(content), options);
        std::error_code error;
        if (!std::filesystem::create_directory(dictionary->root, error))
        {
            const auto message = "Cannot create content directory: " + (
                error ? error.message() : "path already exists");
            product.output->fail(message);
            return std::unexpected(message);
        }

        product.dictionaries.push_back(dictionary);
        return DictionaryWriter(std::make_unique<DictionaryWriter::Impl>(std::move(dictionary)));
    }

    Result<void> ProductWriter::addAsset(std::string_view relative, std::span<const uint8_t> bytes) const
    {
        return impl_->product->writeAsset(relative, bytes);
    }

    Result<void> ProductWriter::setInformation(ProductInformation information) const
    {
        auto& product = *impl_->product;
        if (auto state = product.output->check(); !state)
            return state;

        if (auto valid = detail::ProductInformationWriter::validate(information); !valid)
            return product.output->fail(valid.error());

        product.information = std::move(information);
        return {};
    }

    Result<void> ProductWriter::setIcon(std::span<const uint8_t> image) const
    {
        auto& product = *impl_->product;
        if (auto state = product.output->check(); !state)
            return state;

        if (auto written = detail::ProductIcon::write(product, image); !written)
            return product.output->fail(written.error());

        return {};
    }

    Result<void> ProductWriter::finish() const
    {
        auto& product = *impl_->product;
        if (auto state = product.output->check(); !state)
            return state;

        if (product.dictionaries.empty())
            return product.output->fail("Product has no dictionaries");

        std::set<SearchScope> scopes;
        for (const auto& dictionary : product.dictionaries)
        {
            if (!dictionary->finished)
                return product.output->fail(
                    "Finish every dictionary before finishing the product");

            product.metadata.contents.push_back(dictionary->metadata);
            for (const auto& [scope, writer] : dictionary->keys)
                scopes.insert(scope);
        }

        if (product.profile == CreationProfile::Kanwa)
        {
            for (const auto& scope : detail::native::search::kanwaScopes)
                if (scopes.contains(scope.scope))
                    product.metadata.searchScopeNames.emplace_back(scope.name);
        }
        else
            for (auto scope : scopes)
                product.metadata.searchScopeNames.emplace_back(detail::NativeSearchScope::forScope(scope)->name);

        std::ranges::sort(product.metadata.fonts);
        if (auto result = detail::ProductInformationWriter::write(product); !result)
            return product.output->fail(result.error());

        if (auto result = product.writeAppendices(); !result)
            return product.output->fail(result.error());

        if (auto result = detail::CreationMetadata::writeProduct(product.output->path(), product.name, product.metadata)
            ; !result)
            return product.output->fail(result.error());

        return product.output->publish();
    }
}
