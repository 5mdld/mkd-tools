//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include <MKD/resource/appendix_entry_list.hpp>
#include <optional>
#include <string>

namespace MKD::Python
{
    // Appendix labels borrow storage in C++. Python values own their strings.
    struct AppendixItem
    {
        EntryId entryId;
        std::optional<std::string> label;
        std::optional<std::string> firstCharacter;
        uint16_t dictionaryId = 0;

        [[nodiscard]] AppendixEntry asNative() const;
        [[nodiscard]] static AppendixItem copy(const AppendixEntry& entry);
    };
}
