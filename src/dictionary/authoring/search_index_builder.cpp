//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#include "search_index_builder.hpp"
#include "MKD/dictionary/product_writer.hpp"
#include "reference_index.hpp"
#include "dictionary/search/search_definitions.hpp"
#include "dictionary/search/key_analysis.hpp"
#include "dictionary/package/package_format.hpp"
#include "io/write_validation.hpp"
#include "unicode/unicode.hpp"
#include <limits>

namespace MKD::detail
{
    Result<void> SearchIndexBuilder::addSearchKey(SearchScope scope, std::string_view key, EntryId target,
                                                  SearchKeyOptions options, AuthoringOrigin origin)
    {
        if (auto valid = WriteValidation::entryId(target); !valid)
            return valid;
        if (auto valid = WriteValidation::text(key); !valid)
            return valid;

        const auto profile = profile_;
        if (profile == CreationProfile::Kanwa && scope == SearchScope::Kanji)
            scope = SearchScope::Headword;

        const auto normalized = unicode::normalizeDictionaryKey(key);
        auto info = search::scopeDefinition(scope, profile);
        if (!info)
            return std::unexpected(info.error());

        const bool passage = profile == CreationProfile::Kanwa && !options.exactOnly &&
            (scope == SearchScope::Sense || scope == SearchScope::Example);
        if (passage)
        {
            if (target.itemId > std::numeric_limits<uint16_t>::max() - monokakido::search::normalizedHeadlineOffset)
                return std::unexpected("Entry " + target.toString() +
                    " cannot be used for sense or example search because its item ID is too high to generate "
                    "the required search record. Allocate a target with the entry writer or choose a lower item ID.");

            if (const auto found = passages_.find(target); found != passages_.end() && found->second.scope != scope)
                return std::unexpected("Sense and example search keys need distinct targets, but both use entry " +
                    target.toString() + ". Assign a separate entry target to each.");
        }

        auto it = keys_.find(scope);
        if (it == keys_.end())
        {
            auto writer =
                KeystoreWriter::create(root_ / monokakido::paths::keys / (std::string(info->filename) + ".keystore"));
            if (!writer)
                return std::unexpected(writer.error());
            it = keys_.emplace(scope, std::move(*writer)).first;
        }

        const auto add = [&](std::string_view value, SearchKeyOptions keyOptions) -> Result<void>
        {
            if (auto written = it->second.add(value, target, keyOptions); !written)
                return std::unexpected("Search key '" + std::string(value) + "' in " + std::string(info->filename) +
                    " for " + target.toString() + ": " + written.error());
            return {};
        };

        if (auto written = add(normalized, options); !written)
            return written;
        if (!passage)
            return {};

        for (const auto& component : search::keyComponents(std::span{&normalized, size_t{1}}).values)
            if (auto written = add(component, {}); !written)
                return written;

        auto& passageIndex = passages_.try_emplace(target, PassageIndex{scope, {}, {}}).first->second;
        passageIndex.terms.insert(normalized);
        passageIndex.origins.add(std::move(origin));
        return {};
    }

    void SearchIndexBuilder::validatePassages(const ReferenceIndex& references, IssueCollector& issues,
                                              const DictionaryContent& dictionary) const
    {
        for (const auto& [target, passage] : passages_)
        {
            const auto companion = search::companionTarget(target, monokakido::search::normalizedHeadlineOffset);
            const std::string scope = passage.scope == SearchScope::Sense ? "Sense" : "Example";
            if (references.contains(companion) || references.references(companion))
                issues.add("passage-companion-collision", dictionary, companion.toString(),
                           scope + " search for entry " + target.toString() + " reserves ID " + companion.toString() +
                           " for an automatically generated search record, but this ID is already used or referenced "
                           "by authored content. Assign a different target to the search keys or remove the conflicting "
                           "use of " + companion.toString() + ".", passage.origins);
            if (!primary_.contains(target))
                issues.add("missing-passage-headline", dictionary, target.toString(),
                           scope + " search targets entry " + target.toString() +
                           ", which has no primary headline. Add a primary headline for this entry.", passage.origins);
        }
    }

    Result<void> SearchIndexBuilder::writePassageHeadlines()
    {
        for (const auto& [target, passage] : passages_)
        {
            std::string text;
            for (const auto& term : passage.terms)
            {
                if (!text.empty())
                    text += ' ';
                text += term;
            }

            // Passage filtering reads this record directly. Only the passage itself is a jump target.
            const auto companion = search::companionTarget(target, monokakido::search::normalizedHeadlineOffset);
            if (auto written = headlines_.at(HeadlineRole::Primary)
                                         .add({
                                             .headline = unicode::toUtf16(text),
                                             .sortingHeadline = unicode::toUtf16(text),
                                             .entryId = companion
                                         });
                !written)
                return written;
        }
        return {};
    }

    Result<void> SearchIndexBuilder::addHeadline(HeadlineComponents headline, HeadlineRole role)
    {
        if (auto valid = WriteValidation::entryId(headline.entryId); !valid)
            return valid;

        const auto& names = monokakido::paths::headlineFiles;
        const auto index = static_cast<size_t>(role);
        if (index >= names.size())
            return std::unexpected("Invalid headline role");

        auto it = headlines_.find(role);
        if (it == headlines_.end())
        {
            auto writer = HeadlineStoreWriter::create(root_ / monokakido::paths::headlines / names[index]);
            if (!writer)
                return std::unexpected(writer.error());

            it = headlines_.emplace(role, std::move(*writer)).first;
        }

        if (role == HeadlineRole::Primary)
            primary_.insert(headline.entryId);

        return it->second.add(std::move(headline));
    }

    bool SearchIndexBuilder::hasHeadwords() const
    {
        const auto found = keys_.find(SearchScope::Headword);
        return found != keys_.end() && !found->second.empty();
    }

    std::set<SearchScope> SearchIndexBuilder::scopes() const
    {
        std::set<SearchScope> result;
        for (const auto& scope : keys_ | std::views::keys)
            result.insert(scope);
        return result;
    }

    Result<void> SearchIndexBuilder::finish()
    {
        if (auto result = writePassageHeadlines(); !result)
            return result;

        for (auto& writer : headlines_ | std::views::values)
            if (auto result = writer.finish(); !result)
                return result;

        for (auto& writer : keys_ | std::views::values)
            if (auto result = writer.finish(); !result)
                return result;

        return {};
    }
}
