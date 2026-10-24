#include "creation_session.hpp"
#include "creation_metadata.hpp"
#include "io/write_validation.hpp"
#include "search/key_analysis.hpp"
#include "unicode/unicode.hpp"

#include <limits>

namespace MKD::detail
{
    Result<void> DictionaryBuild::addSearchKey(SearchScope scope, std::string_view key, EntryId target,
                                               SearchKeyOptions options)
    {
        if (auto valid = WriteValidation::entryId(target); !valid)
            return valid;
        if (auto valid = WriteValidation::text(key); !valid)
            return valid;

        const auto profile = product.lock()->profile;
        if (profile == CreationProfile::Kanwa && scope == SearchScope::Kanji)
            scope = SearchScope::Headword;

        const auto normalized = unicode::normalizeDictionaryKey(key);
        auto info = CreationMetadata::scopeDefinition(scope, profile);
        if (!info)
            return std::unexpected(info.error());

        const bool passage = profile == CreationProfile::Kanwa && !options.exactOnly &&
            (scope == SearchScope::Sense || scope == SearchScope::Example);
        if (passage)
        {
            if (target.itemId > std::numeric_limits<uint16_t>::max() - native::search::normalizedHeadlineOffset)
                return std::unexpected("Native passage companion overflows target " + target.toString());

            if (const auto found = passageIndexes.find(target);
                found != passageIndexes.end() && found->second.scope != scope)
                return std::unexpected("Native sense and example indexes need distinct targets: " + target.toString());
        }

        auto it = keys.find(scope);
        if (it == keys.end())
        {
            auto writer = KeystoreWriter::create(root / native::paths::keys /
                                                (std::string(info->filename) + ".keystore"));
            if (!writer)
                return std::unexpected(writer.error());
            it = keys.emplace(scope, std::move(*writer)).first;
        }

        const auto add = [&](std::string_view value, SearchKeyOptions keyOptions) -> Result<void>
        {
            if (auto written = it->second.add(value, target, keyOptions); !written)
                return std::unexpected("Search key '" + std::string(value) + "' in " +
                    std::string(info->filename) + " for " + target.toString() + ": " + written.error());
            return {};
        };

        if (auto written = add(normalized, options); !written)
            return written;
        references.insert(target);
        if (!passage)
            return {};

        for (const auto& component : search::keyComponents(std::span{&normalized, size_t{1}}).values)
            if (auto written = add(component, {}); !written)
                return written;

        passageIndexes.try_emplace(target, PassageIndex{scope, {}}).first->second.terms.insert(normalized);
        return {};
    }

    Result<void> DictionaryBuild::writePassageHeadlines()
    {
        for (const auto& [target, passage] : passageIndexes)
        {
            const auto companion = search::companionTarget(target, native::search::normalizedHeadlineOffset);
            if (anchors.contains(companion) || references.contains(companion))
                return std::unexpected("Native passage companion " + companion.toString() +
                    " for " + target.toString() + " collides with an article target or headline");
            if (!primaryHeadlines.contains(target))
                return std::unexpected("Native passage target needs a primary headline: " + target.toString());
        }

        for (const auto& [target, passage] : passageIndexes)
        {
            std::string text;
            for (const auto& term : passage.terms)
            {
                if (!text.empty())
                    text += ' ';
                text += term;
            }

            // Native filtering reads this record directly. Only the passage itself is a jump target.
            const auto companion = search::companionTarget(target, native::search::normalizedHeadlineOffset);
            if (auto written = headlines.at(HeadlineRole::Primary).add({
                    .headline = unicode::toUtf16(text), .sortingHeadline = unicode::toUtf16(text),
                    .entryId = companion}); !written)
                return written;
        }
        return {};
    }
}
