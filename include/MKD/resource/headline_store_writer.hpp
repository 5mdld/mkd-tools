//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/resource/headline_store.hpp"

namespace MKD
{
    class HeadlineStoreWriter
    {
    public:
        [[nodiscard]] static Result<HeadlineStoreWriter> create(const std::filesystem::path& path);
        ~HeadlineStoreWriter();
        HeadlineStoreWriter(HeadlineStoreWriter&&) noexcept;
        HeadlineStoreWriter& operator=(HeadlineStoreWriter&&) noexcept;
        [[nodiscard]] Result<void> add(HeadlineComponents headline) const;
        [[nodiscard]] Result<void> finish() const;
        [[nodiscard]] size_t size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
        explicit HeadlineStoreWriter(std::unique_ptr<Impl> impl);
    };
}
