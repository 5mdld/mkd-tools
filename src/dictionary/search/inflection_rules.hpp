//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/dictionary/detail/metadata_keys.hpp"

#include <array>
#include <span>
#include <string_view>
#include <utility>

namespace MKD::detail::search::inflections
{
    using Rule = std::pair<std::string_view, std::string_view>;
    using Rules = std::span<const Rule>;

    inline constexpr auto english = std::to_array<Rule>({
        {"s", ""}, {"es", ""}, {"ies", "y"}, {"ves", "fe"}, {"cy", "t"}, {"ily", "y"}, {"lly", "ll"}, {"uly", "ue"}
    });
    inline constexpr auto pastAndProgressive = std::to_array<Rule>({
        {"ed", ""}, {"ed", "e"}, {"ied", "y"}, {"gged", "g"}, {"bbed", "b"}, {"pped", "p"}, {"rred", "r"},
        {"tted", "t"}, {"lled", "l"}, {"cked", "c"}, {"ing", ""}, {"ing", "e"}, {"gging", "g"}, {"bbing", "b"},
        {"pping", "p"}, {"rring", "r"}, {"tting", "t"}
    });
    inline constexpr auto comparative = std::to_array<Rule>({
        {"er", ""}, {"est", ""}, {"er", "e"}, {"est", "e"}, {"gger", "g"}, {"ggest", "g"}, {"tter", "t"},
        {"ttest", "t"}, {"nner", "n"}, {"nnest", "n"}, {"ier", "y"}, {"iest", "y"}
    });
    inline constexpr auto french = std::to_array<Rule>({
        {"s", ""}, {"es", ""}, {"ies", "y"}, {"ves", "fe"}, {"ily", "y"}, {"y", "e"}, {"lly", "ll"}, {"uly", "ue"}
    });
    inline constexpr auto italian = std::to_array<Rule>({
        {"i", "o"}, {"e", "a"}, {"a", "o"}, {"e", "o"}, {"i", "e"}
    });
    inline constexpr auto portuguese = std::to_array<Rule>({
        {"s", ""}, {"ns", "m"}, {"res", "r"}, {"ses", "s"}, {"zes", "s"}, {"is", "l"}
    });
    inline constexpr auto spanish = std::to_array<Rule>({
        {"a", "o"}, {"a", ""}, {"s", ""}, {"es", ""}, {"ces", "z"}, {"os", "o"}, {"as", "a"},
        {"isimo", "o"}, {"isima", "a"}, {"isimos", "os"}, {"isimas", "as"}, {"isimo", ""}, {"isoma", ""},
        {"isimos", ""}, {"isimas", ""}, {"ncisimo", "n"}, {"rcisimo", "r"}, {"bilisimo", "ble"}
    });
    inline constexpr auto german = std::to_array<Rule>({
        {"te", "en"}, {"e", ""}, {"st", ""}, {"t", ""}, {"en", ""}, {"le", "eln"}, {"dest", "den"}, {"det", "den"},
        {"test", "ten"}, {"tet", "ten"}, {"mest", "men"}, {"met", "men"}, {"nest", "nen"}, {"net", "nen"},
        {"sst", "ssen"}, {"ßt", "ßen"}, {"zt", "zen"}, {"xt", "xen"}, {"ten", "en"}, {"test", "en"}, {"tet", "en"},
        {"er", ""}, {"n", ""}, {"s", ""}, {"es", "e"}, {"els", "el"}, {"ens", "en"}, {"ers", "er"}, {"ses", "s"},
        {"sses", "ss"}, {"ßes", "ß"}, {"zes", "z"}, {"tzes", "tz"}, {"e", "en"}, {"es", ""}
    });

    constexpr std::array<Rules, 3> forLanguage(std::string_view language)
    {
        if (language.starts_with(metadata::language::japanese) || language.starts_with(metadata::language::english))
            return {english, pastAndProgressive, comparative};
        if (language.starts_with(metadata::language::french)) return {french, pastAndProgressive, Rules{}};
        if (language.starts_with(metadata::language::italian)) return {italian, french, pastAndProgressive};
        if (language.starts_with(metadata::language::german)) return {german, Rules{}, Rules{}};
        if (language.starts_with(metadata::language::spanish)) return {spanish, Rules{}, Rules{}};
        if (language.starts_with(metadata::language::portuguese)) return {portuguese, Rules{}, Rules{}};
        return {};
    }
}
