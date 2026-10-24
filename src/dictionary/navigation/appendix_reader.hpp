//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#pragma once

#include "MKD/dictionary/appendix_menu.hpp"
#include "MKD/result.hpp"
#include <filesystem>
#include <set>
#include <unordered_map>
#include <pugixml.hpp>

namespace MKD::detail
{
    inline constexpr size_t maximumAppendixMenuDepth = 64;

    class AppendixReader
    {
    public:
        static Result<AppendixMenu> open(const std::filesystem::path& contents);
        static std::unordered_map<std::string, std::string> listTitles(const AppendixMenu& menu);
        static Result<std::filesystem::path> resolve(const std::filesystem::path& contents, std::string_view path);

    private:
        explicit AppendixReader(std::filesystem::path contents) : contents_(std::move(contents))
        {
        }

        Result<AppendixMenu> read(std::string_view path);
        Result<AppendixGallery> gallery(std::string_view path) const;
        Result<AppendixItem> item(pugi::xml_node node);
        Result<AppendixTarget> target(pugi::xml_node node);
        std::filesystem::path contents_;
        std::set<std::filesystem::path> reading_;
    };
}
