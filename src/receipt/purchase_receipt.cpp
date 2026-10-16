//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#include "MKD/receipt/purchase_receipt.hpp"
#include "MKD/platform/mmap_file.hpp"
#include "platform/macos/chacha_poly.hpp"

#include <glaze/glaze.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <format>
#include <fstream>
#include <map>
#include <utility>

namespace MKD::detail
{
    struct StoredPurchase : Purchase
    {
        std::map<std::string, glz::raw_json> extra;
    };

    struct ReceiptData : ReceiptMetadata
    {
        std::map<std::string, StoredPurchase, std::less<>> purchases;
        std::map<std::string, glz::raw_json> extra;
    };
}

template <>
struct glz::meta<MKD::detail::StoredPurchase>
{
    using T = MKD::detail::StoredPurchase;
    static constexpr auto value = object(
        "product_id", &T::productId,
        "transaction_id", &T::transactionId,
        "original_transaction_id", &T::originalTransactionId,
        "purchase_date", &T::purchaseDate
    );
    static constexpr auto unknown_read = &T::extra;
    static constexpr auto unknown_write = &T::extra;
};

template <>
struct glz::meta<MKD::detail::ReceiptData>
{
    using T = MKD::detail::ReceiptData;
    static constexpr auto value = object(
        "isSandbox", &T::isSandbox,
        "receiptIdentifier", &T::receiptIdentifier,
        "vendorIdentifier", &T::vendorIdentifier,
        "version", &T::version,
        "purchases", &T::purchases,
        "revokedTransactions", &T::revokedTransactions,
        "unitedPurchases", &T::unitedPurchases,
        "uploadedTransactions", &T::uploadedTransactions
    );
    static constexpr auto unknown_read = &T::extra;
    static constexpr auto unknown_write = &T::extra;
};

namespace MKD
{
    struct PurchaseReceipt::Impl
    {
        // The purchases format uses this fixed key.
        static constexpr std::array<uint8_t, 32> key = {
            'r', 'H', 'D', 'M', 'g', 'l', '1', 'P', 'S', 'W', 'o', 'r', 'X', 'j', 'q', 'g',
            'j', 'S', 'v', 'o', 'u', 'C', 'L', 'x', 'w', 'a', 'n', 'W', 's', 'H', 'S', 'A'
        };
        detail::ReceiptData data;
    };

    PurchaseReceipt::PurchaseReceipt(std::unique_ptr<Impl> impl) noexcept
        : impl_(std::move(impl))
    {
    }

    PurchaseReceipt::~PurchaseReceipt() = default;
    PurchaseReceipt::PurchaseReceipt(PurchaseReceipt&&) noexcept = default;
    PurchaseReceipt& PurchaseReceipt::operator=(PurchaseReceipt&&) noexcept = default;

    Result<PurchaseReceipt> PurchaseReceipt::open(const std::filesystem::path& path)
    {
        auto file = MappedFile::open(path);
        if (!file)
            return std::unexpected(file.error());

        auto receipt = decode(file->data());
        if (!receipt)
            return std::unexpected(std::format("Cannot decode '{}': {}", path.string(), receipt.error()));

        return receipt;
    }

    Result<PurchaseReceipt> PurchaseReceipt::decode(std::span<const uint8_t> bytes)
    {
        auto plaintext = detail::openChaChaPoly(bytes, Impl::key);
        if (!plaintext)
            return std::unexpected(plaintext.error());

        auto impl = std::make_unique<Impl>();
        const std::span json(reinterpret_cast<const char*>(plaintext->data()), plaintext->size());
        if (const auto error = glz::read<glz::opts{.error_on_unknown_keys = false}>(impl->data, json))
            return std::unexpected("Failed to parse receipt JSON: " + glz::format_error(error, json));

        return PurchaseReceipt(std::move(impl));
    }

    Result<std::vector<uint8_t>> PurchaseReceipt::encode() const
    {
        std::string json;
        if (const auto error = glz::write_json(impl_->data, json))
            return std::unexpected("Failed to serialize receipt JSON: " + glz::format_error(error, json));

        return detail::sealChaChaPoly(
            {reinterpret_cast<const uint8_t*>(json.data()), json.size()}, Impl::key);
    }

    Result<void> PurchaseReceipt::save(const std::filesystem::path& path) const
    {
        auto bytes = encode();
        if (!bytes)
            return std::unexpected(bytes.error());

        auto temporary = path;
        temporary += std::format(".mkd-{}.tmp", std::chrono::steady_clock::now().time_since_epoch().count());
        std::ofstream file(temporary, std::ios::binary | std::ios::out | std::ios::noreplace);
        if (!file)
            return std::unexpected(std::format("Cannot create temporary receipt '{}': {}", temporary.string(),
                                               std::make_error_code(std::errc::io_error).message()));

        struct PendingFile
        {
            const std::filesystem::path& path;
            std::ofstream& file;

            ~PendingFile()
            {
                if (file.is_open()) file.close();
                std::error_code error;
                std::filesystem::remove(path, error);
            }
        } pending{temporary, file};

        file.write(reinterpret_cast<const char*>(bytes->data()), static_cast<std::streamsize>(bytes->size()));
        file.close();
        if (!file)
            return std::unexpected(std::format("Failed to write receipt '{}'", temporary.string()));

        std::error_code error;
        const auto status = std::filesystem::status(path, error);
        if (error && error != std::errc::no_such_file_or_directory)
            return std::unexpected(std::format("Cannot inspect '{}': {}", path.string(), error.message()));

        if (std::filesystem::exists(status))
        {
            std::filesystem::permissions(temporary, status.permissions(), error);
            if (error)
                return std::unexpected(std::format("Cannot set receipt permissions: {}", error.message()));
        }

        std::filesystem::rename(temporary, path, error);
        if (error)
            return std::unexpected(std::format("Cannot replace '{}': {}", path.string(), error.message()));

        return {};
    }

    const Purchase* PurchaseReceipt::findPurchase(std::string_view productId) const
    {
        const auto direct = impl_->data.purchases.find(productId);
        if (direct != impl_->data.purchases.end() && direct->second.productId == productId)
            return &direct->second;
        const auto found = std::ranges::find_if(impl_->data.purchases, [productId](const auto& entry)
        {
            return entry.second.productId == productId;
        });
        return found == impl_->data.purchases.end() ? nullptr : &found->second;
    }

    bool PurchaseReceipt::contains(std::string_view productId) const
    {
        return findPurchase(productId) || std::ranges::find(impl_->data.unitedPurchases, productId)
            != impl_->data.unitedPurchases.end();
    }

    Result<void> PurchaseReceipt::addPurchase(Purchase purchase)
    {
        if (purchase.productId.empty() || purchase.transactionId.empty() || purchase.originalTransactionId.empty()
            || purchase.purchaseDate.empty())
            return std::unexpected("Purchase fields must not be empty");
        if (findPurchase(purchase.productId))
            return std::unexpected("Purchase already exists: " + purchase.productId);

        const auto id = purchase.productId;
        if (!impl_->data.purchases.emplace(id, detail::StoredPurchase{std::move(purchase), {}}).second)
            return std::unexpected("Purchase map key already exists: " + id);

        return {};
    }

    const ReceiptMetadata& PurchaseReceipt::metadata() const noexcept { return impl_->data; }
    ReceiptMetadata& PurchaseReceipt::metadata() noexcept { return impl_->data; }
}
