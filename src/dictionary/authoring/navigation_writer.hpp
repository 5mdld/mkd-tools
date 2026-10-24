//
// kiwakiwaaにより 2026/10/19 に作成されました。
//

#pragma once

#include "MKD/dictionary/navigation.hpp"
#include "binary/binary_buffer.hpp"
#include "MKD/resource/resource_store.hpp"
#include <set>
#include <map>
#include <pugixml.hpp>

namespace MKD::detail
{
    struct ProductBuild;

    class NavigationWriter
    {
    public:
        static Result<void> write(ProductBuild& product, const NavigationIndex& index);

    private:
        struct Node
        {
            NavigationLabel title = std::string{};
            uint8_t kind = 0;
            uint16_t flags = 0;
            std::vector<Node> children;
            std::vector<NavigationItem> entries;
            std::vector<std::pair<std::string, std::string>> files;
            std::vector<uint32_t> sectionPath;
        };

        class Encoder
        {
        public:
            explicit Encoder(uint32_t kind);
            Result<void> node(const Node& node, size_t base);
            std::span<const uint8_t> bytes() const { return bytes_; }

        private:
            uint32_t text(std::string_view text, size_t base);
            void patch(size_t offset, const BinaryBuffer& record);
            uint32_t append(const BinaryBuffer& record, size_t base);
            std::vector<uint8_t> bytes_;
        };

        explicit NavigationWriter(ProductBuild& product);
        Result<std::string> index(const NavigationIndex& index, std::string title, size_t depth,
                                  std::optional<std::string> filename = std::nullopt);
        Result<Node> binary(const NavigationIndex& index, std::string title, size_t depth);
        Result<Node> section(const NavigationSection& section, size_t depth, NavigationKind kind);
        Result<void> plist(const NavigationIndex& index, std::string_view title, std::string_view path, size_t depth);
        Result<void> plistItem(pugi::xml_node parent, const NavigationItem& item, size_t depth);
        Result<std::string> targetFile(const NavigationItem& item, size_t depth, std::string title = {});
        Result<AppendixEntryLink> entry(AppendixEntryLink target);
        Result<void> label(const NavigationLabel& label);
        Result<void> save(std::string_view path, std::span<const uint8_t> data) const;
        Result<NavigationIndex> appendices(const AppendixMenu& menu, size_t depth);
        Result<NavigationIndex> entryList(const AppendixList& list) const;
        Result<std::string> gallery(const AppendixGallery& gallery);
        std::string nextPath(std::string_view extension);
        std::string indexRelative(std::string_view path) const;
        ProductBuild& product_;
        std::filesystem::path contents_;
        std::string directory_;
        bool kanwa_;
        size_t next_ = 0;
        std::map<uint16_t, ResourceStore> stores_;
        std::set<std::pair<uint16_t, EntryId>> checked_;
        std::map<uint32_t, std::pair<std::string, std::string>> types_;
    };
}
