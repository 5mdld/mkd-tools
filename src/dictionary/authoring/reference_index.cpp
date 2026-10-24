//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#include "reference_index.hpp"
#include "dictionary/content/analysis_support.hpp"
#include "dictionary/package/paths.hpp"
#include <algorithm>

namespace MKD::detail
{
    Result<void> ReferenceIndex::record(const content::References& references, AuthoringOrigin sourceOrigin,
                                        std::string_view sourcePath, std::string_view input)
    {
        if (references.empty()) return {};
        const SourceMap source(input);
        for (const auto& reference : references)
        {
            auto location = sourceOrigin;
            if (reference.source)
            {
                const auto [line, column] = source.lineColumn(reference.source->offset);
                location.location = SourceLocation{reference.source->offset, reference.source->length, line, column};
            }
            const auto remember = [&](auto& index, auto&& key)
            {
                index[std::forward<decltype(key)>(key)].add(location);
            };
            if (const auto target = std::get_if<EntryId>(&reference.target))
                remember(targets_, *target);
            else if (const auto asset = std::get_if<content::AssetReference>(&reference.target))
            {
                auto path = resolveAssetReference(*asset, sourcePath);
                if (!path)
                    return std::unexpected(location.describe() + ": " + path.error());
                if (*path)
                    remember(assets_, std::move(**path));
            }
            else if (const auto gaiji = std::get_if<content::GaijiReference>(&reference.target))
                remember(gaiji_, gaiji->name);
        }
        return {};
    }

    void ReferenceIndex::define(const std::set<EntryId>& targets)
    {
        anchors_.insert(targets.begin(), targets.end());
    }

    void ReferenceIndex::requireTarget(EntryId target, AuthoringOrigin origin)
    {
        targets_[target].add(std::move(origin));
    }

    void ReferenceIndex::requireAsset(std::string path, AuthoringOrigin origin)
    {
        assets_[std::move(path)].add(std::move(origin));
    }

    void ReferenceIndex::validateTargets(IssueCollector& issues, const DictionaryContent& dictionary) const
    {
        for (const auto& [target, origins] : targets_)
            if (!contains(target))
                issues.add("missing-entry-target", dictionary, target.toString(),
                           "Missing entry target " + target.toString(), origins);
    }

    std::vector<EntryId> ReferenceIndex::takeTargets() &&
    {
        std::vector<EntryId> targets(anchors_.begin(), anchors_.end());
        std::ranges::sort(targets);
        return targets;
    }
}
