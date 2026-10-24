//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <pugixml.hpp>

namespace MKD::detail
{
    class PropertyListDictionary
    {
    public:
        explicit PropertyListDictionary(pugi::xml_node node) : node_(node)
        {
        }

        [[nodiscard]] pugi::xml_node value(std::string_view key) const
        {
            for (auto node : node_.children("key"))
                if (node.child_value() == key)
                    return node.next_sibling();
            return {};
        }

        [[nodiscard]] std::optional<std::string> text(std::string_view key) const
        {
            const auto node = value(key);
            return node && std::string_view(node.name()) == "string"
                       ? std::optional<std::string>(node.child_value())
                       : std::nullopt;
        }

        [[nodiscard]] bool flag(std::string_view key) const
        {
            const auto node = value(key);
            return std::string_view(node.name()) == "true" ||
                (std::string_view(node.name()) == "integer" && node.text().as_llong() != 0);
        }

        [[nodiscard]] Result<uint32_t> integer(std::string_view key) const
        {
            const auto node = value(key);
            const std::string_view text = node.child_value();
            uint32_t number{};
            const auto parsed = std::from_chars(text.data(), text.data() + text.size(), number);
            if (!node || parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
                return std::unexpected("Invalid integer in property list: " + std::string(key));

            return number;
        }

    private:
        pugi::xml_node node_;
    };

    class PropertyList
    {
    public:
        static Result<PropertyList> open(const std::filesystem::path& path, std::string_view rootType)
        {
            PropertyList result;
            std::ifstream file(path, std::ios::binary);
            if (!file)
                return std::unexpected("Cannot open property list: " + path.string());

            const auto parsed = result.document_.load(file);
            if (!parsed)
                return std::unexpected("Cannot read property list: " + path.string() + ": " + parsed.description());

            if (std::string_view(result.root().name()) != rootType)
                return std::unexpected("Expected property list " + std::string(rootType) + ": " + path.string());

            return result;
        }

        [[nodiscard]] pugi::xml_node root() const { return document_.child("plist").first_child(); }
        [[nodiscard]] PropertyListDictionary dictionary() const { return PropertyListDictionary(root()); }

    private:
        pugi::xml_document document_;
    };
}

namespace MKD
{
    struct LocalizedString;
}

namespace MKD::detail::property_list
{
    pugi::xml_node root(pugi::xml_document& document, std::string_view kind);
    void string(pugi::xml_node dictionary, std::string_view key, std::string_view value);
    void integer(pugi::xml_node dictionary, std::string_view key, uint64_t value);
    void boolean(pugi::xml_node dictionary, std::string_view key, bool value);
    void localized(pugi::xml_node dictionary, std::string_view key, const std::optional<LocalizedString>& value);
    Result<void> write(const std::filesystem::path& path, pugi::xml_document& document);
}
