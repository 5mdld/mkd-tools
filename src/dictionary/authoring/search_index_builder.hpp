//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#pragma once

#include "MKD/dictionary/dictionary_writer.hpp"
#include "authoring_diagnostics.hpp"

namespace MKD
{
    enum class CreationProfile;
}

#include "MKD/resource/headline_store_writer.hpp"
#include <map>
#include <set>
#include <unordered_set>

namespace MKD::detail
{
    class ReferenceIndex;

    class SearchIndexBuilder
    {
    public:
        SearchIndexBuilder(std::filesystem::path root, const CreationProfile profile)
            : root_(std::move(root)), profile_(profile)
        {
        }

        Result<void> addHeadline(HeadlineComponents headline, HeadlineRole role);
        Result<void> addSearchKey(SearchScope scope, std::string_view key, EntryId target, SearchKeyOptions options,
                                  AuthoringOrigin origin);
        void validatePassages(const ReferenceIndex& references, IssueCollector& issues,
                              const DictionaryContent& dictionary) const;
        Result<void> finish();
        [[nodiscard]] bool hasPrimary(EntryId target) const { return primary_.contains(target); }
        [[nodiscard]] size_t primaryCount() const { return primary_.size(); }
        [[nodiscard]] bool hasHeadwords() const;
        [[nodiscard]] std::set<SearchScope> scopes() const;

    private:
        Result<void> writePassageHeadlines();

        struct PassageIndex
        {
            SearchScope scope;
            std::set<std::string> terms;
            DependencyOrigins origins;
        };

        std::filesystem::path root_;
        CreationProfile profile_;
        std::map<HeadlineRole, HeadlineStoreWriter> headlines_;
        std::map<SearchScope, KeystoreWriter> keys_;
        std::map<EntryId, PassageIndex> passages_;
        std::unordered_set<EntryId> primary_;
    };
}
