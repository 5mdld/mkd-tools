//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "bindings.hpp"

NB_MODULE(_mkd, module)
{
    namespace py = MKD::Python;
    py::nb::exception<py::Error>(module, "Error", PyExc_RuntimeError);
    py::bindEntryId(module);
    py::bindLocalizedString(module);
    py::bindPresentation(module);
    py::bindContent(module);
    py::bindMetadata(module);
    py::bindHeadline(module);
    py::bindInlineElements(module);
    py::bindInlineText(module);
    py::bindAppendixEntry(module);
    py::bindBuffer(module);
    py::bindResourceStores(module);
    py::bindFont(module);
    py::bindStylesheet(module);
    py::bindHeadlineStore(module);
    py::bindKeystore(module);
    py::bindDiscovery(module);
    py::bindAppendixTargets(module);
    py::bindAppendixCustom(module);
    py::bindAppendixMenu(module);
    py::bindProduct(module);
    py::bindAppendixList(module);
    py::bindEntryOrder(module);
    py::bindDictionary(module);
    py::bindSearchEnums(module);
    py::bindSearchMatch(module);
    py::bindSearchResult(module);
    py::bindSearch(module);
    py::bindExport(module);
    py::bindCreationMetadata(module);
    py::bindProductWriter(module);
    py::bindEntryWriter(module);
    py::bindDictionaryWriter(module);
}
