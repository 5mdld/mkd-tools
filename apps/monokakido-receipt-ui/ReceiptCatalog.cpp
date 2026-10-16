//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#include "ReceiptCatalog.hpp"

#include "MKD/dictionary/dictionary_product.hpp"
#include "MKD/platform/directory_dictionary_source.hpp"
#include "MKD/receipt/purchase_receipt.hpp"
#include "MKD/platform/macos/paths.hpp"

#include <algorithm>
#include <chrono>
#include <format>
#include <exception>
#include <ranges>
#include <set>
#include <utility>

namespace MKDReceipt
{
    ReceiptCatalog::ReceiptCatalog(std::string dataDirectory)
        : dataDirectory_(std::move(dataDirectory))
    {
    }

    Outcome<std::string> ReceiptCatalog::defaultDataDirectory() try
    {
        auto directory = MKD::macOS::monokakidoDataDirectory();
        if (!directory)
            return {std::nullopt, directory.error()};
        return {directory->string(), {}};
    }
    catch (const std::exception& error)
    {
        return {std::nullopt, error.what()};
    }

    Outcome<std::vector<Item>> ReceiptCatalog::loadInstalledProducts() const
    {
        const MKD::DirectoryDictionarySource source(MKD::macOS::dictionariesDirectory(dataDirectory_));
        auto dictionaries = source.findAllAvailable();
        if (!dictionaries)
            return {std::nullopt, dictionaries.error()};

        std::vector<Item> installed;
        installed.reserve(dictionaries->size());
        for (const auto& dictionary : *dictionaries)
        {
            auto product = MKD::DictionaryProduct::loadMetadata(dictionary.path);
            if (!product)
                return {std::nullopt, product.error()};

            Item item{
                .dictionaryId = dictionary.id,
                .productId = product->metadata().productIdentifier,
                .title = product->displayTitle(),
            };
            if (const auto icon = product->iconPath())
                item.iconPath = icon->string();

            installed.push_back(std::move(item));
        }
        return {std::move(installed), {}};
    }

    Outcome<Snapshot> ReceiptCatalog::inspect() const try
    {
        auto installed = loadInstalledProducts();
        if (!installed.value)
            return {std::nullopt, std::move(installed.error)};

        auto receipt = MKD::PurchaseReceipt::open(MKD::macOS::purchasesPath(dataDirectory_));
        if (!receipt)
            return {std::nullopt, receipt.error()};

        std::set<std::string> missingProducts;
        for (auto& item : *installed.value)
        {
            item.hasReceipt = receipt->contains(item.productId);
            if (const auto* purchase = receipt->findPurchase(item.productId);
                purchase && !purchase->purchaseDate.empty())
                item.purchaseDate = purchase->purchaseDate;
            if (!item.hasReceipt)
                missingProducts.insert(item.productId);
        }
        return {Snapshot{std::move(*installed.value), missingProducts.size()}, {}};
    }
    catch (const std::exception& error)
    {
        // Swift cannot catch a C++ exception crossing the interop boundary.
        return {std::nullopt, error.what()};
    }

    Outcome<size_t> ReceiptCatalog::generateMissing() const try
    {
        auto installed = loadInstalledProducts();
        if (!installed.value)
            return {std::nullopt, std::move(installed.error)};

        const auto path = MKD::macOS::purchasesPath(dataDirectory_);
        auto receipt = MKD::PurchaseReceipt::open(path);
        if (!receipt)
            return {std::nullopt, receipt.error()};

        const auto now = std::chrono::system_clock::now();
        const auto date = std::format("{:%Y-%m-%dT%H:%M:%SZ}", std::chrono::floor<std::chrono::seconds>(now));
        const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        auto& metadata = receipt->metadata();
        size_t generatedCount = 0;
        size_t sequence = 0;
        for (const auto& item : *installed.value)
        {
            if (receipt->contains(item.productId))
                continue;

            std::string transactionId;
            do
            {
                transactionId = std::format("{}{:03}", millis, ++sequence);
            }
            while (std::ranges::find(metadata.uploadedTransactions, transactionId)
                != metadata.uploadedTransactions.end());
            auto added = receipt->addPurchase(MKD::Purchase{
                .productId = item.productId,
                .transactionId = transactionId,
                .originalTransactionId = transactionId,
                .purchaseDate = date,
            });
            if (!added)
                return {std::nullopt, added.error()};
            metadata.unitedPurchases.push_back(item.productId);
            metadata.uploadedTransactions.push_back(std::move(transactionId));
            ++generatedCount;
        }
        if (generatedCount > 0)
        {
            if (auto saved = receipt->save(path); !saved)
                return {std::nullopt, saved.error()};
        }
        return {generatedCount, {}};
    }
    catch (const std::exception& error)
    {
        return {std::nullopt, error.what()};
    }
}
