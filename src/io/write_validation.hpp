//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/resource/entry_id.hpp"
#include "MKD/result.hpp"
#include "unicode/utf8.hpp"

#include <filesystem>
#include <string_view>

namespace MKD::detail
{
    struct WriteValidation
    {
        static Result<void> xmlText(std::string_view value, bool allowEmpty = false)
        {
            if (auto valid = text(value, allowEmpty); !valid)
                return valid;

            size_t offset = 0;
            while (offset < value.size())
            {
                const auto cp = unicode::nextCodepoint(value, offset);
                if ((cp < 0x20 && cp != '\t' && cp != '\n' && cp != '\r') || cp == 0xFFFE || cp == 0xFFFF)
                    return std::unexpected("Text contains a character forbidden by XML 1.0");
            }
            return {};
        }

        static Result<void> text(std::u16string_view value, bool allowEmpty = false)
        {
            if ((!allowEmpty && value.empty()) || value.contains(u'\0') || unicode::firstInvalidUtf16Offset(value))
                return std::unexpected("Expected valid UTF-16 text without embedded nulls");
            return {};
        }

        static Result<void> text(std::string_view value, bool allowEmpty = false)
        {
            if ((!allowEmpty && value.empty()) || value.contains('\0') || unicode::firstInvalidUtf8Offset(value))
                return std::unexpected("Expected valid UTF-8 text without embedded nulls");
            return {};
        }

        static Result<void> relativePath(std::string_view value)
        {
            if (auto valid = text(value); !valid)
                return valid;

            if (value.contains('\\') || value.contains(':'))
                return std::unexpected("Invalid relative asset path");

            const auto path = fromUtf8(value);
            if (path.is_absolute() || path.has_root_name())
                return std::unexpected("Asset path must be relative");

            for (const auto& part : path)
                if (part.empty() || part == "." || part == "..")
                    return std::unexpected("Asset path must not contain empty, dot or parent components");

            return {};
        }

        static Result<void> entryId(const EntryId id)
        {
            if (id.pageId == 0 || id.pageId > 0xFFFFFF)
                return std::unexpected("Entry page ID must be between 1 and 16777215");
            return {};
        }

        static std::filesystem::path fromUtf8(std::string_view value)
        {
            return {
                std::u8string_view(reinterpret_cast<const char8_t*>(value.data()), value.size())
            };
        }
    };
}
