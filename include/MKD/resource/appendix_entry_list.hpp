//
// kiwakiwaaにより 2026/05/01 に作成されました。
//

#pragma once

#include "MKD/resource/entry_id.hpp"
#include "MKD/result.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>


namespace MKD
{
    struct AppendixEntry
    {
        EntryId entryId;
        std::optional<std::string_view> label;
        std::optional<char16_t> firstCharacter;
        /** Native target dictionary. Zero selects the owner during dictionary creation. */
        uint16_t dictionaryId = 0;
    };

    /** An appendix entry matrix. Entry labels are borrowed from the list. */
    class AppendixEntryList
    {
    public:
        static Result<AppendixEntryList> open(const std::filesystem::path& filePath, std::string title = {});

        ~AppendixEntryList();

        AppendixEntryList(AppendixEntryList&&) noexcept;
        AppendixEntryList& operator=(AppendixEntryList&&) noexcept;

        AppendixEntryList(const AppendixEntryList&) = delete;
        AppendixEntryList& operator=(const AppendixEntryList&) = delete;

        [[nodiscard]] size_t size() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] uint32_t rows() const noexcept;

        [[nodiscard]] uint32_t columns() const noexcept;

        [[nodiscard]] bool hasLabels() const noexcept;

        [[nodiscard]] bool hasFirstCharacters() const noexcept;

        [[nodiscard]] std::string_view filename() const noexcept;

        [[nodiscard]] std::string_view title() const noexcept;

        [[nodiscard]] Result<AppendixEntry> entryAt(size_t index) const;

    private:
        struct Impl;

        explicit AppendixEntryList(std::unique_ptr<Impl> impl);

        std::unique_ptr<Impl> impl_;
    };
}
