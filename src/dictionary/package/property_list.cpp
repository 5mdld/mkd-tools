//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#include "property_list.hpp"
#include "MKD/dictionary/metadata.hpp"
#include "io/output_file.hpp"

namespace MKD::detail::property_list
{
    pugi::xml_node root(pugi::xml_document& document, std::string_view kind)
    {
        auto declaration = document.append_child(pugi::node_declaration);
        declaration.append_attribute("version") = "1.0";
        declaration.append_attribute("encoding") = "UTF-8";
        auto plist = document.append_child("plist");
        plist.append_attribute("version") = "1.0";
        return plist.append_child(std::string(kind).c_str());
    }

    void string(pugi::xml_node dictionary, std::string_view key, std::string_view value)
    {
        dictionary.append_child("key").text().set(std::string(key).c_str());
        dictionary.append_child("string").text().set(std::string(value).c_str());
    }

    void integer(pugi::xml_node dictionary, std::string_view key, const uint64_t value)
    {
        dictionary.append_child("key").text().set(std::string(key).c_str());
        dictionary.append_child("integer").text().set(std::to_string(value).c_str());
    }

    void boolean(pugi::xml_node dictionary, std::string_view key, bool value)
    {
        dictionary.append_child("key").text().set(std::string(key).c_str());
        dictionary.append_child(value ? "true" : "false");
    }

    void localized(pugi::xml_node dictionary, std::string_view key, const std::optional<LocalizedString>& value)
    {
        if (!value)
            return;

        dictionary.append_child("key").text().set(std::string(key).c_str());

        auto languages = dictionary.append_child("dict");
        if (value->en)
            string(languages, metadata::language::english, *value->en);
        if (value->ja)
            string(languages, metadata::language::japanese, *value->ja);
    }

    Result<void> write(const std::filesystem::path& path, pugi::xml_document& document)
    {
        OutputFile file(path, false);
        if (!file.isOpen())
            return std::unexpected("Cannot create property list: " + path.string());

        document.save(file.stream(), "  ", pugi::format_default, pugi::encoding_utf8);
        return file.finish().transform([](uint64_t)
        {
        });
    }
}
