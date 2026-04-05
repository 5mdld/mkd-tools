//
// kiwakiwaaにより 2026/01/30 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include "MKD/resource/retained_span.hpp"

#include <expected>
#include <filesystem>
#include <iterator>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

namespace MKD
{
    struct ResourceStoreItem
    {
        uint32_t itemId;
        RetainedSpan data;

        /**
         * Validate that the payload is UTF-8 and return a non-owning view.
         * The returned view remains valid while this ResourceStoreItem is alive.
         */
        [[nodiscard]] Result<std::string_view> asUtf8StringView() const;

        /**
         * Validate UTF-8 and return an owning std::string copy.
         */
        [[nodiscard]] Result<std::string> asUtf8String() const;
    };

    class ResourceStore
    {
    public:
        ~ResourceStore() noexcept;
        ResourceStore(ResourceStore&&) noexcept;
        ResourceStore& operator=(ResourceStore&&) noexcept;

        /**
         * Factory method to open .rsc dictionary contents from a dictionary
         *
         * @param directoryPath Directory path containing the .idx/.map & .rsc files
         * @param dictId Needed if the dictionary has encryption applied. The decryption key is derived from the identifier
         * @return
         */
        static Result<ResourceStore> open(const fs::path& directoryPath, std::string_view dictId = "");

        /**
         * This will basically look up the itemId first with RscIndex, if it finds a
         * map record, it will use that to retrieve the data from the .rsc
         * this works well for dictionary entries where you want to get the xml data for a given itemId
         */
        [[nodiscard]] Result<RetainedSpan> get(uint32_t itemId) const;

        /**
         * Get RscItem by index
         * @param index index
         * @return Rscitem or error string if failure
         */
        [[nodiscard]] Result<ResourceStoreItem> getByIndex(size_t index) const;

        /**
         * Get total number of records
         * @return Number of records
         */
        [[nodiscard]] size_t size() const noexcept;

        /**
         * @return true if there are no records
         */
        [[nodiscard]] bool empty() const noexcept;


        class Iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using iterator_concept = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = ResourceStoreItem;
            using pointer = value_type*;
            using reference = value_type;

            Iterator() noexcept = default;
            Iterator(const ResourceStore* rsc, size_t index);

            value_type operator*() const;

            Iterator& operator++();
            Iterator operator++(int);

            bool operator==(const Iterator& other) const;
            bool operator!=(const Iterator& other) const;

        private:
            const ResourceStore* store_ = nullptr;
            size_t index_ = 0;
        };

        [[nodiscard]] Iterator begin() const;
        [[nodiscard]] Iterator end() const;

        static_assert(std::forward_iterator<Iterator>);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;

        explicit ResourceStore(std::unique_ptr<Impl> impl) noexcept;
    };


}
