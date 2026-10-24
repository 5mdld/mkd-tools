//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#pragma once

#include "MKD/dictionary/content_analysis.hpp"
#include "MKD/result.hpp"

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace MKD::detail
{
    class ReferenceIndex
    {
    public:
        Result<void> record(const content::References& references, std::string_view origin,
                            std::string_view sourcePath = {}, std::string_view input = {});
        void define(const std::set<EntryId>& targets);
        void requireTarget(EntryId target, std::string origin);
        void requireAsset(std::string path, std::string origin);
        [[nodiscard]] bool contains(const EntryId target) const { return anchors_.contains(target); }
        [[nodiscard]] bool references(const EntryId target) const { return targets_.contains(target); }
        Result<void> validateTargets() const;
        [[nodiscard]] const auto& requiredAssets() const { return assets_; }
        [[nodiscard]] const auto& requiredGaiji() const { return gaiji_; }
        std::vector<EntryId> takeTargets() &&;

    private:
        std::unordered_set<EntryId> anchors_;
        std::unordered_map<EntryId, std::string> targets_;
        std::map<std::string, std::string, std::less<>> assets_;
        std::map<std::string, std::string, std::less<>> gaiji_;
    };
}
