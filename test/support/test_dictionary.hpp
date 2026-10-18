//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#pragma once

#include "MKD/mkd.hpp"
#include <gtest/gtest.h>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace MKD::test
{
    template <typename T>
    T require(Result<T> result)
    {
        if (!result)
            throw std::runtime_error(result.error());
        if constexpr (!std::is_void_v<T>)
            return std::move(*result);
    }

    std::filesystem::path dictionaries();
    std::filesystem::path assets();
    std::span<const uint8_t> bytes(std::string_view text);
    std::vector<uint8_t> readFile(const std::filesystem::path& path);
    void writeFile(const std::filesystem::path& path, std::span<const uint8_t> data);
    DictionaryContent birbContent();

    class Workspace
    {
    public:
        Workspace();
        ~Workspace();
        Workspace(const Workspace&) = delete;
        Workspace& operator=(const Workspace&) = delete;
        std::filesystem::path path;
    };

    struct BirbBuild
    {
        explicit BirbBuild(const std::filesystem::path& path, bool encrypted = false);
        EntryId addBirb(std::optional<uint32_t> pageId = std::nullopt);
        void finish();
        ProductWriter product;
        DictionaryWriter dictionary;
    };

    class DictionaryTest : public ::testing::Test
    {
    protected:
        DictionaryProduct product = require(DictionaryProduct::openAtPath(dictionaries() / "Birbs"));
        const Dictionary& birbs() const { return product.dictionaries().front(); }
    };
}
