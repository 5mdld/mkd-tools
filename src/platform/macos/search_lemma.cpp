//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "platform/search_lemma.hpp"

#include <Foundation/Foundation.hpp>

namespace MKD::detail
{
    std::optional<std::string> searchLemma(std::string_view text, std::string_view language)
    {
        NS::ScopedAutoreleasePool pool;
        auto* word = NS::String::alloc()->initWithBytes(text.data(), text.size(), NS::UTF8StringEncoding);
        if (!word)
            return std::nullopt;

        word->autorelease();
        auto* name = NS::String::alloc()->initWithBytes(language.data(), language.size(), NS::UTF8StringEncoding);
        if (!name)
            return std::nullopt;

        name->autorelease();
        const auto* orthography = NS::Orthography::defaultOrthographyForLanguage(name);
        if (!orthography || word->length() < 2)
            return std::nullopt;

        const auto* tags = word->linguisticTagsInRange({0, word->length()}, NS::LinguisticTagSchemeLemma,
                                                       NS::LinguisticTaggerOmitWhitespace, orthography, nullptr);
        if (!tags || !tags->count())
            return std::nullopt;

        const auto* lemma = static_cast<const NS::String*>(tags->objectAtIndex(0));
        if (!lemma || !lemma->length())
            return std::nullopt;

        const auto* result = lemma->utf8String();
        return result ? std::optional<std::string>(result) : std::nullopt;
    }
}
