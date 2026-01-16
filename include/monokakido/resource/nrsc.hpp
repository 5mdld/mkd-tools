//
// Caoimheにより 2026/01/16 に作成されました。
//

#pragma once

#include "nrsc_index.hpp"

#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>

#include "nrsc.hpp"

namespace fs = std::filesystem;

namespace monokakido::resource
{
    class NrscData;


    // Resource item that can be returned to users
    struct ResourceItem
    {
        std::string_view id;
        std::span<const uint8_t> data;
    };


    class Nrsc
    {
    public:
        /**
         * Factory method to open a .nrsc resource from a directory
         * @param directoryPath Directory path containing the resources
         * @return Nrsc resource class or error string if failure
         */
        static std::expected<Nrsc, std::string> open(const fs::path& directoryPath);


        /**
         * Get resource by string ID
         * - Calls findById from NrscIndex to do a binary search in the index
         * - O(log(n))
         * @param id String ID of the resource
         * @return Data view of resource data, error string if failure
         */
        [[nodiscard]] std::expected<std::span<const uint8_t>, std::string> get(std::string_view id);


        /**
         * Get resource by index
         * - Calls getByIndex from NrscIndex
         * - O(1)
         * @param index
         * @return
         */
        [[nodiscard]] std::expected<ResourceItem, std::string> getByIndex(size_t index);


        /**
         * Get total number of resources
         * @return Number of resources
         */
        [[nodiscard]] size_t size() const noexcept;

    private:

        explicit Nrsc(NrscIndex&& index, NrscData&& data);

        NrscIndex index_;
        //NrscData data_;

    };

}