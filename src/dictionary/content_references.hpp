//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "native_content.hpp"
#include <set>
#include <filesystem>

namespace MKD::detail
{
    struct ContentReferences
    {
        std::set<EntryId> targets;
        std::set<std::string> assets;
        std::set<std::string> gaijiNames;
        static Result<ContentReferences> headline(std::u16string_view text);
    };

    class CssReferences
    {
    public:
        static Result<std::set<std::string>> read(std::string_view path, std::string_view css);

    private:
        std::string_view source_;
        size_t position_ = 0;
        std::filesystem::path directory_;
        std::set<std::string> paths_;
        Result<void> whitespace();
        Result<std::string> string(char quote);
        Result<void> escape(std::string& value);
        Result<void> reference(std::string_view value);
        Result<void> url();
        Result<void> scan();
    };
}
