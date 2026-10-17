//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/resource/appendix_entry_list.hpp"

namespace MKD
{
    struct AppendixListWriteOptions
    {
        uint32_t rows = 0;
        uint32_t columns = 1;
    };

    /** Rows are inferred from the entry count when rows is zero. */
    class AppendixEntryListWriter
    {
    public:
        [[nodiscard]] static Result<AppendixEntryListWriter> create(const std::filesystem::path& path,
                                                                    AppendixListWriteOptions options = {});
        ~AppendixEntryListWriter();
        AppendixEntryListWriter(AppendixEntryListWriter&&) noexcept;
        AppendixEntryListWriter& operator=(AppendixEntryListWriter&&) noexcept;
        [[nodiscard]] Result<void> add(const AppendixEntry& entry) const;
        [[nodiscard]] Result<void> finish() const;
        [[nodiscard]] size_t size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
        explicit AppendixEntryListWriter(std::unique_ptr<Impl> impl);
    };
}
