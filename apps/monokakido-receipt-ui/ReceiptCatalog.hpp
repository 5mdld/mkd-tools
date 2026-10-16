//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace MKDReceipt
{
    template <typename T>
    struct Outcome
    {
        std::optional<T> value;
        std::string error;
    };

    struct Item
    {
        std::string dictionaryId;
        std::string productId;
        std::string title;
        std::optional<std::string> purchaseDate;
        std::optional<std::string> iconPath;
        bool hasReceipt = false;
    };

    struct Snapshot
    {
        std::vector<Item> items;
        size_t missingCount = 0;
    };

    class ReceiptCatalog
    {
    public:
        explicit ReceiptCatalog(std::string dataDirectory);

        static Outcome<std::string> defaultDataDirectory();
        [[nodiscard]] Outcome<Snapshot> inspect() const;
        [[nodiscard]] Outcome<size_t> generateMissing() const;

    private:
        [[nodiscard]] Outcome<std::vector<Item>> loadInstalledProducts() const;

        std::string dataDirectory_;
    };
}
