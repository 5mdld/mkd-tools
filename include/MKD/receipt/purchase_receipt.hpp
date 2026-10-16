//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#pragma once

#include "MKD/result.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace MKD
{
    struct Purchase
    {
        std::string productId;
        std::string transactionId;
        std::string originalTransactionId;
        std::string purchaseDate;
    };

    struct ReceiptMetadata
    {
        bool isSandbox = false;
        std::string receiptIdentifier;
        std::string vendorIdentifier;
        std::string version;
        std::vector<std::string> revokedTransactions;
        std::vector<std::string> unitedPurchases;
        std::vector<std::string> uploadedTransactions;
    };

    class PurchaseReceipt
    {
    public:
        ~PurchaseReceipt();
        PurchaseReceipt(PurchaseReceipt&&) noexcept;
        PurchaseReceipt& operator=(PurchaseReceipt&&) noexcept;

        static Result<PurchaseReceipt> open(const std::filesystem::path& path);
        static Result<PurchaseReceipt> decode(std::span<const uint8_t> bytes);

        [[nodiscard]] Result<std::vector<uint8_t>> encode() const;
        /** Replaces the destination only after the temporary file has been written and closed. */
        Result<void> save(const std::filesystem::path& path) const;

        /** Includes compacted entries in unitedPurchases. */
        [[nodiscard]] bool contains(std::string_view productId) const;
        /** Returns a borrowed detailed record, or nullptr for a missing or compacted entry. */
        [[nodiscard]] const Purchase* findPurchase(std::string_view productId) const;
        /**
         * Adds a detailed record without altering metadata.
         * Existing detailed records and occupied map keys are rejected.
         */
        Result<void> addPurchase(Purchase purchase);

        [[nodiscard]] const ReceiptMetadata& metadata() const noexcept;
        [[nodiscard]] ReceiptMetadata& metadata() noexcept;

    private:
        struct Impl;
        explicit PurchaseReceipt(std::unique_ptr<Impl> impl) noexcept;
        std::unique_ptr<Impl> impl_;
    };
}
