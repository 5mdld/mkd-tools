//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#pragma once

#include "MKD/dictionary/content_analysis.hpp"
#include <optional>

namespace MKD::detail
{
    class SourceMap
    {
    public:
        explicit SourceMap(std::string_view input);
        [[nodiscard]] std::string describe(std::string_view origin,
                                           std::optional<content::SourceRange> range) const;

    private:
        size_t size_;
        std::vector<size_t> lines_{0};
    };

    inline std::optional<content::Reference> resourceReference(std::string_view value, const content::ResourceSyntax syntax,
                                                               std::optional<content::SourceRange> source)
    {
        if (value.empty())
            return std::nullopt;

        if (value.contains(':') || value.starts_with("//"))
            return content::Reference{content::ExternalReference{std::string(value)}, source};

        if (value.starts_with('#'))
            return content::Reference{content::FragmentReference{std::string(value.substr(1)), syntax}, source};

        return content::Reference{content::AssetReference{std::string(value), syntax}, source};
    }
}
