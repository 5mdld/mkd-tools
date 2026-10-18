//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#pragma once

#include "MKD/dictionary/appendix_menu.hpp"
#include "MKD/result.hpp"
#include <filesystem>
#include <pugixml.hpp>

namespace MKD::detail
{
    struct ProductBuild;

    class AppendixWriter
    {
    public:
        static Result<void> validateEntry(const ProductBuild& product, const AppendixEntryLink& entry);
        static Result<void> write(const ProductBuild& product, const AppendixMenu& menu);

    private:
        explicit AppendixWriter(const ProductBuild& product) : product_(product)
        {
        }

        Result<void> menu(const AppendixMenu& menu, std::string_view path);
        Result<void> item(pugi::xml_node node, const AppendixItem& item);
        Result<void> target(pugi::xml_node node, const AppendixList& list) const;
        Result<void> target(pugi::xml_node node, const AppendixDocument& document) const;
        Result<void> target(pugi::xml_node node, const AppendixEntryLink& entry) const;
        Result<void> target(pugi::xml_node node, const AppendixMenu& submenu);
        static Result<void> target(pugi::xml_node node, const AppendixCustom& custom);
        Result<void> target(pugi::xml_node node, const AppendixGallery& gallery);
        Result<void> file(std::string_view path, std::string_view extension) const;
        const ProductBuild& product_;
        size_t nextMenu_ = 0;
        size_t depth_ = 0;
    };
}
