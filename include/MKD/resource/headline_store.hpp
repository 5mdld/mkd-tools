//
// kiwakiwaaにより 2026/02/12 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include "MKD/resource/entry_id.hpp"

#include <filesystem>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>

namespace MKD
{
    struct HeadlineComponents
    {
        std::u16string prefix; // empty if absent
        std::u16string headline;
        std::u16string suffix; // empty if absent
        std::u16string sortingHeadline;
        EntryId entryId;

        [[nodiscard]] std::u16string full() const;
    };


    class HeadlineStore
    {
    public:
        static Result<HeadlineStore> open(const std::filesystem::path& path);
        ~HeadlineStore();
        HeadlineStore(HeadlineStore&&) noexcept;
        HeadlineStore& operator=(HeadlineStore&&) noexcept;

        /** Only the requested entry is decoded. Returned component strings are owned. */
        [[nodiscard]] Result<HeadlineComponents> entryAt(size_t index) const;
        [[nodiscard]] Result<EntryId> entryIdAt(size_t index) const;
        [[nodiscard]] Result<HeadlineComponents> componentsForEntryId(const EntryId& id,
                                                                      bool allowFallback = true) const;
        [[nodiscard]] Result<std::u16string> sortingHeadlineForEntryId(const EntryId& id) const;
        [[nodiscard]] size_t size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;
        [[nodiscard]] std::string_view filename() const noexcept;

        [[nodiscard]] auto entries() const &
        {
            return std::views::iota(size_t{0}, size())
                | std::views::transform([this](const size_t index) { return entryAt(index); });
        }

        auto entries() const && = delete;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
        explicit HeadlineStore(std::unique_ptr<Impl> impl) noexcept;
    };
}
