//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "bindings.hpp"
#include <MKD/dictionary/metadata.hpp>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace MKD::Python
{
    void bindLocalizedString(nb::module_& module)
    {
        nb::enum_<LocalizedLanguage>(module, "LocalizedLanguage")
            .value("JAPANESE", LocalizedLanguage::Japanese)
            .value("ENGLISH", LocalizedLanguage::English);

        auto type = nb::class_<LocalizedString>(module, "LocalizedString");
        type.def(nb::init<std::optional<std::string>, std::optional<std::string>>(), nb::kw_only(),
                 "en"_a = nb::none(), "ja"_a = nb::none());

        type.def_rw("en", &LocalizedString::en).def_rw("ja", &LocalizedString::ja);

        type.def("resolve", nb::overload_cast<>(&LocalizedString::resolve, nb::const_));

        type.def("resolve", nb::overload_cast<LocalizedLanguage>(&LocalizedString::resolve, nb::const_),
                 "language"_a);
    }

    void bindPresentation(nb::module_& module)
    {
        auto type = nb::class_<DictionaryPresentation>(module, "DictionaryPresentation");
        type.def(nb::init<std::optional<std::string>>(), nb::kw_only(),
                 "full_ruby_stylesheet"_a = nb::none());

        type.def_rw("full_ruby_stylesheet", &DictionaryPresentation::fullRubyStylesheet);
    }

    void bindContent(nb::module_& module)
    {
        auto type = nb::class_<DictionaryContent>(module, "DictionaryContent");
        type.def(nb::init<std::string, std::string, std::optional<uint16_t>, std::optional<std::string>,
                          std::optional<LocalizedString>, std::optional<LocalizedString>,
                          std::optional<LocalizedString>, std::optional<std::vector<std::string>>,
                          std::optional<std::string>, std::optional<bool>, DictionaryPresentation>(),
                 nb::kw_only(), "identifier"_a = "", "directory"_a = "", "dictionary_id"_a = nb::none(),
                 "version"_a = nb::none(), "title"_a = nb::none(), "publisher"_a = nb::none(),
                 "edition"_a = nb::none(), "languages"_a = nb::none(), "key_normalization"_a = nb::none(),
                 "has_headline_key_identifier"_a = nb::none(), "presentation"_a = DictionaryPresentation{});

        type.def_rw("identifier", &DictionaryContent::identifier);
        type.def_rw("directory", &DictionaryContent::directory);
        type.def_rw("dictionary_id", &DictionaryContent::dictionaryId);
        type.def_rw("version", &DictionaryContent::version);
        type.def_rw("title", &DictionaryContent::title);
        type.def_rw("publisher", &DictionaryContent::publisher);
        type.def_rw("edition", &DictionaryContent::edition);
        type.def_rw("languages", &DictionaryContent::languages);
        type.def_rw("key_normalization", &DictionaryContent::keyNormalization);
        type.def_rw("has_headline_key_identifier", &DictionaryContent::hasHeadlineKeyIdentifier);
        type.def_rw("presentation", &DictionaryContent::presentation);
    }

    void bindMetadata(nb::module_& module)
    {
        auto type = nb::class_<DictionaryMetadata>(module, "DictionaryMetadata");
        type.def(
            nb::init<std::optional<LocalizedString>, std::optional<LocalizedString>,
                     std::optional<LocalizedString>, std::optional<LocalizedString>, std::string,
                     std::optional<std::string>, std::optional<std::string>, std::optional<std::string>,
                     std::optional<std::string>, std::vector<DictionaryContent>, std::vector<std::string>,
                     std::vector<std::string>, std::optional<bool>, std::optional<std::string>>(),
            nb::kw_only(), "display_name"_a = nb::none(), "description"_a = nb::none(),
            "genre"_a = nb::none(), "product_title"_a = nb::none(), "product_identifier"_a = "",
            "category"_a = nb::none(), "version"_a = nb::none(), "product_search_class"_a = nb::none(),
            "product_delegate_class"_a = nb::none(), "contents"_a = std::vector<DictionaryContent>{},
            "search_scope_names"_a = std::vector<std::string>{}, "fonts"_a = std::vector<std::string>{},
            "vertical_layout"_a = nb::none(), "required_dic_kit_version"_a = nb::none());

        type.def_rw("display_name", &DictionaryMetadata::displayName);
        type.def_rw("description", &DictionaryMetadata::description);
        type.def_rw("genre", &DictionaryMetadata::genre);
        type.def_rw("product_title", &DictionaryMetadata::productTitle);
        type.def_rw("product_identifier", &DictionaryMetadata::productIdentifier);
        type.def_rw("category", &DictionaryMetadata::category);
        type.def_rw("version", &DictionaryMetadata::version);
        type.def_rw("product_search_class", &DictionaryMetadata::productSearchClass);
        type.def_rw("product_delegate_class", &DictionaryMetadata::productDelegateClass);
        type.def_rw("contents", &DictionaryMetadata::contents);
        type.def_rw("search_scope_names", &DictionaryMetadata::searchScopeNames);
        type.def_rw("fonts", &DictionaryMetadata::fonts);
        type.def_rw("vertical_layout", &DictionaryMetadata::verticalLayout);
        type.def_ro("appendix_view_controller", &DictionaryMetadata::appendixViewController);
        type.def_ro("contents_view_controller", &DictionaryMetadata::contentsViewController);
        type.def_ro("label_formatter", &DictionaryMetadata::labelFormatter);
        type.def_ro("appendix_first", &DictionaryMetadata::appendixFirst);
        type.def_rw("required_dic_kit_version", &DictionaryMetadata::requiredDicKitVersion);
    }
}
