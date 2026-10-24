//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#include "reference_index.hpp"
#include "dictionary/content/analysis_support.hpp"
#include "dictionary/package/paths.hpp"
#include <algorithm>

namespace MKD::detail
{
    Result<void> ReferenceIndex::record(const content::References& references, std::string_view origin,
                                        std::string_view sourcePath, std::string_view input)
    {
        if (references.empty()) return {};
        const SourceMap source(input);
        for (const auto& reference : references)
        {
            auto location = source.describe(origin, reference.source);
            const auto remember = [&](auto& index, auto&& key)
            {
                if (reference.source)
                    index.insert_or_assign(std::forward<decltype(key)>(key), std::move(location));
                else
                    index.try_emplace(std::forward<decltype(key)>(key), std::move(location));
            };
            if (const auto target = std::get_if<EntryId>(&reference.target))
                remember(targets_, *target);
            else if (const auto asset = std::get_if<content::AssetReference>(&reference.target))
            {
                auto path = resolveAssetReference(*asset, sourcePath);
                if (!path)
                    return std::unexpected(location + ": " + path.error());
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

    void ReferenceIndex::requireTarget(EntryId target, std::string origin)
    {
        targets_.try_emplace(target, std::move(origin));
    }

    void ReferenceIndex::requireAsset(std::string path, std::string origin)
    {
        assets_.try_emplace(std::move(path), std::move(origin));
    }

    Result<void> ReferenceIndex::validateTargets() const
    {
        for (const auto& [target, origin] : targets_)
            if (!contains(target))
                return std::unexpected("Missing entry target " + target.toString() + " referenced by " + origin);
        return {};
    }

    std::vector<EntryId> ReferenceIndex::takeTargets() &&
    {
        std::vector<EntryId> targets(anchors_.begin(), anchors_.end());
        std::ranges::sort(targets);
        return targets;
    }
}
