//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include <pugixml.h>
#include <filesystem>
#include <optional>
#include <unordered_map>

namespace MKD::detail
{
    class AppendixMetadata
    {
        using AppendixTitleMap = std::unordered_map<std::string, std::string>;

        static pugi::xml_node nextElementSibling(pugi::xml_node node)
        {
            for (node = node.next_sibling(); node; node = node.next_sibling())
            {
                if (node.type() == pugi::node_element)
                    return node;
            }
            return {};
        }

        static std::optional<std::string> plistStringValue(pugi::xml_node dict, std::string_view key)
        {
            for (auto node = dict.first_child(); node; node = node.next_sibling())
            {
                if (node.type() != pugi::node_element || std::string_view(node.name()) != "key")
                    continue;

                if (std::string_view(node.child_value()) != key)
                    continue;

                const auto value = nextElementSibling(node);
                if (value && std::string_view(value.name()) == "string")
                    return value.child_value();

                return std::nullopt;
            }

            return std::nullopt;
        }

        static void collectAppendixEntryTitles(pugi::xml_node node, AppendixTitleMap& titles)
        {
            if (node.type() == pugi::node_element && std::string_view(node.name()) == "dict")
            {
                const auto contents = plistStringValue(node, "contents");
                if (contents && contents->ends_with(".entries"))
                {
                    if (auto name = plistStringValue(node, "name"); name && !name->empty())
                        titles.emplace(*contents, std::move(*name));
                }
            }

            for (auto child = node.first_child(); child; child = child.next_sibling())
                collectAppendixEntryTitles(child, titles);
        }

    public:
        static Result<AppendixTitleMap> open(const std::filesystem::path& productRoot)
        {
            const auto path = productRoot / "Contents" / "Appendix.plist";
            std::error_code ec;
            const auto status = std::filesystem::status(path, ec);
            if (ec == std::errc::no_such_file_or_directory || (!ec && !std::filesystem::exists(status)))
                return AppendixTitleMap{};
            if (ec)
                return std::unexpected("Cannot inspect appendix metadata: " + path.string() + ": " + ec.message());
            if (!std::filesystem::is_regular_file(status))
                return std::unexpected("Appendix metadata is not a regular file: " + path.string());

            pugi::xml_document doc;
            const auto pathString = path.string();
            if (auto result = doc.load_file(pathString.c_str()); !result)
                return std::unexpected("Cannot read appendix metadata: " + path.string() + ": " + result.description());

            AppendixTitleMap titles;
            collectAppendixEntryTitles(doc.document_element(), titles);
            return titles;
        }
    };
}
