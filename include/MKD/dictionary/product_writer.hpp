//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/dictionary/dictionary_writer.hpp"

namespace MKD
{
    enum class CreationProfile
    {
        General,
        Kanwa
    };

    /** Builds a new native product without replacing an existing destination. */
    class ProductWriter
    {
    public:
        [[nodiscard]] static Result<ProductWriter> create(const std::filesystem::path& destination,
                                                          DictionaryMetadata metadata,
                                                          CreationProfile profile = CreationProfile::General);
        ~ProductWriter();
        ProductWriter(ProductWriter&&) noexcept;
        ProductWriter& operator=(ProductWriter&&) noexcept;

        [[nodiscard]] Result<DictionaryWriter> addDictionary(DictionaryContent content,
                                                             DictionaryWriteOptions options = {}) const;
        [[nodiscard]] Result<void> addAsset(std::string_view relativePath, std::span<const uint8_t> bytes) const;
        [[nodiscard]] Result<void> finish() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
        explicit ProductWriter(std::unique_ptr<Impl> impl);
    };
}
