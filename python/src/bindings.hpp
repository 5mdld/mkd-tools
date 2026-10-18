//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include <MKD/result.hpp>
#include <nanobind/nanobind.h>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace MKD::Python
{
    namespace nb = nanobind;
    using namespace nb::literals;

    class Error : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    template <typename T>
    T unwrap(Result<T> result)
    {
        if (!result)
            throw Error(result.error());

        if constexpr (!std::is_void_v<T>)
            return std::move(*result);
    }

    void bindEntryId(nb::module_ & module);
    void bindLocalizedString(nb::module_ & module);
    void bindPresentation(nb::module_ & module);
    void bindContent(nb::module_ & module);
    void bindMetadata(nb::module_ & module);
    void bindHeadline(nb::module_ & module);
    void bindInlineElements(nb::module_ & module);
    void bindInlineText(nb::module_ & module);
    void bindAppendixTargets(nb::module_& module);
    void bindAppendixCustom(nb::module_& module);
    void bindAppendixMenu(nb::module_& module);
    void bindAppendixEntry(nb::module_ & module);
    void bindBuffer(nb::module_ & module);
    void bindProduct(nb::module_ & module);
    void bindAppendixList(nb::module_ & module);
    void bindDictionary(nb::module_ & module);
    void bindEntryOrder(nb::module_ & module);
    void bindResourceStores(nb::module_ & module);
    void bindFont(nb::module_ & module);
    void bindStylesheet(nb::module_ & module);
    void bindHeadlineStore(nb::module_ & module);
    void bindKeystore(nb::module_ & module);
    void bindDiscovery(nb::module_ & module);
    void bindExport(nb::module_ & module);
    void bindSearchEnums(nb::module_ & module);
    void bindSearchMatch(nb::module_ & module);
    void bindSearchResult(nb::module_ & module);
    void bindSearch(nb::module_ & module);
    void bindCreationMetadata(nb::module_ & module);
    void bindProductWriter(nb::module_ & module);
    void bindDictionaryWriter(nb::module_ & module);
    void bindEntryWriter(nb::module_ & module);
}
