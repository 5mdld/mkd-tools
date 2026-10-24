//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#pragma once

#include "MKD/dictionary/content_analysis.hpp"
#include "MKD/result.hpp"
#include "authoring_diagnostics.hpp"

#include <map>
#include <set>
#include <unordered_set>

namespace MKD::detail
{
    class ReferenceIndex
    {
    public:
        Result<void> record(const content::References& references, AuthoringOrigin origin,
                            std::string_view sourcePath = {}, std::string_view input = {});
        void define(const std::set<EntryId>& targets);
        void requireTarget(EntryId target, AuthoringOrigin origin);
        void requireAsset(std::string path, AuthoringOrigin origin);
        [[nodiscard]] bool contains(const EntryId target) const { return anchors_.contains(target); }
        [[nodiscard]] bool references(const EntryId target) const { return targets_.contains(target); }
        void validateTargets(IssueCollector& issues, const DictionaryContent& dictionary) const;
        [[nodiscard]] const auto& requiredAssets() const { return assets_; }
        [[nodiscard]] const auto& requiredGaiji() const { return gaiji_; }
        std::vector<EntryId> takeTargets() &&;

    private:
        std::unordered_set<EntryId> anchors_;
        std::map<EntryId, DependencyOrigins> targets_;
        std::map<std::string, DependencyOrigins, std::less<>> assets_;
        std::map<std::string, DependencyOrigins, std::less<>> gaiji_;
    };
}
