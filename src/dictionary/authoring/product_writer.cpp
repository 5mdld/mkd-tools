//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "MKD/dictionary/product_writer.hpp"
#include "writer_handles.hpp"
#include "product_build.hpp"

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
                                                const CreationProfile profile)
    {
        auto product = detail::ProductBuild::create(destination, std::move(metadata), profile);
        if (!product)
            return std::unexpected(product.error());

        return ProductWriter(std::make_unique<Impl>(std::move(*product)));
    }

    Result<DictionaryWriter> ProductWriter::addDictionary(DictionaryContent content,
                                                          DictionaryWriteOptions options) const
    {
        auto dictionary = impl_->product->addDictionary(std::move(content), options);
        if (!dictionary)
            return std::unexpected(dictionary.error());

        return DictionaryWriter(std::make_unique<DictionaryWriter::Impl>(std::move(*dictionary)));
    }

    Result<void> ProductWriter::addAsset(std::string_view path, std::span<const uint8_t> bytes) const
    {
        return impl_->product->writeAsset(path, bytes);
    }

    Result<void> ProductWriter::setAppendices(AppendixMenu menu) const
    {
        return impl_->product->setAppendices(std::move(menu));
    }

    Result<void> ProductWriter::setNavigation(NavigationIndex index) const
    {
        return impl_->product->setNavigation(std::move(index));
    }

    Result<void> ProductWriter::setInformation(ProductInformation information) const
    {
        return impl_->product->setInformation(std::move(information));
    }

    Result<void> ProductWriter::setIcon(std::span<const uint8_t> image) const
    {
        return impl_->product->setIcon(image);
    }

    Result<void> ProductWriter::finish() const
    {
        return impl_->product->finish();
    }
}
