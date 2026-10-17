//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/resource/entry_id.hpp"
#include "MKD/result.hpp"
#include <pugixml.hpp>
#include <set>
#include <optional>
#include <string>
#include <vector>

namespace MKD::detail
{
    struct NativeEntryReference
    {
        static std::optional<EntryId> parse(std::string_view text);
    };

    struct PageReferences : pugi::xml_tree_walker
    {
        static Result<PageReferences> read(uint32_t page, std::string_view xhtml);
        bool for_each(pugi::xml_node& node) override;
        std::set<EntryId> anchors;
        std::set<EntryId> targets;
        std::set<std::string> assets;

    private:
        uint32_t page_ = 0;
        std::set<std::string> ids_;
        std::set<std::string> fragments_;
        std::optional<std::string> error_;
        void reference(std::string_view value, bool asset);
    };
}
