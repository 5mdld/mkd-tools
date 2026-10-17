//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "creation_page.hpp"
#include "io/write_validation.hpp"


namespace MKD::detail
{
    Result<PageReferences> PageReferences::read(const uint32_t page, std::string_view xhtml)
    {
        if (auto valid = WriteValidation::xmlText(xhtml); !valid)
            return std::unexpected(valid.error());

        pugi::xml_document document;
        auto parsed = document.load_buffer(xhtml.data(), xhtml.size(), pugi::parse_default, pugi::encoding_utf8);
        if (!parsed || std::string_view(document.document_element().name()) != "html")
            return std::unexpected("Page must be a well-formed UTF-8 XHTML document with an html root");

        for (const auto node : document.children())
            if (node.type() == pugi::node_element && node != document.document_element())
                return std::unexpected("Page must contain exactly one document element");

        PageReferences references;
        references.page_ = page;
        references.anchors.insert({page, 0});
        document.traverse(references);
        if (references.error_)
            return std::unexpected(*references.error_);

        for (const auto& fragment : references.fragments_)
            if (!references.ids_.contains(fragment)) return std::unexpected("Unknown page fragment: " + fragment);

        return references;
    }

    bool PageReferences::for_each(pugi::xml_node& node)
    {
        if (const auto id = node.attribute("id"))
        {
            if (!ids_.insert(id.value()).second)
                error_ = "Duplicate XML id within a page";

            if (const auto target = NativeEntryReference::parse(id.value()))
            {
                if (target->pageId != page_) error_ = "Entry anchor belongs to another page";
                else anchors.insert(*target);
            }
        }

        if (const auto source = node.attribute("src"))
            reference(source.value(), true);

        if (const auto link = node.attribute("href"))
            reference(link.value(), std::string_view(node.name()) == "link");

        return !error_;
    }

    void PageReferences::reference(std::string_view value, bool asset)
    {
        if (value.empty() || value.contains(':') || value.starts_with("//"))
            return;

        if (value.starts_with('#'))
        {
            fragments_.emplace(value.substr(1));
            return;
        }

        if (!asset)
        {
            if (const auto target = NativeEntryReference::parse(value))
            {
                targets.insert(*target);
                return;
            }
        }

        value = value.substr(0, value.find_first_of("#?"));
        if (auto valid = WriteValidation::relativePath(value); !valid)
            error_ = valid.error();

        else assets.emplace(value);
    }
}
