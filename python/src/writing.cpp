//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "bindings.hpp"
#include "appendix.hpp"
#include "buffer.hpp"
#include <MKD/dictionary/product_writer.hpp>
#include <nanobind/stl/filesystem.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/vector.h>

namespace MKD::Python
{
    void addAppendixEntries(const DictionaryWriter& dictionary, std::string_view name, std::string_view title,
                            const std::vector<AppendixItem>& items, const uint32_t rows, const uint32_t columns)
    {
        std::vector<AppendixEntry> entries;
        entries.reserve(items.size());
        for (const auto& item : items)
            entries.push_back(item.asNative());

        unwrap(dictionary.addAppendix(name, title, entries, {.rows = rows, .columns = columns}));
    }

    void bindProductWriter(nb::module_& module)
    {
        auto type = nb::class_<ProductWriter>(module, "ProductWriter",
                                              "Stage a new product. Finish each dictionary, then finish the product to publish.");
        type.def_static(
            "create", [](const std::filesystem::path& destination, DictionaryMetadata metadata)
            {
                return unwrap(ProductWriter::create(destination, std::move(metadata)));
            }, "destination"_a,
            "metadata"_a);

        type.def(
            "add_dictionary", [](const ProductWriter& product, DictionaryContent content, bool encrypt)
            {
                return unwrap(product.addDictionary(std::move(content), {.encryptContents = encrypt}));
            },
            "content"_a, nb::kw_only(), "encrypt_contents"_a = false, nb::keep_alive<0, 1>());

        type.def(
            "add_asset", [](const ProductWriter& product, std::string_view path, nb::handle data)
            {
                unwrap(product.addAsset(path, InputBytes(data).span()));
            }, "path"_a, "data"_a);

        type.def("finish", [](const ProductWriter& product) { unwrap(product.finish()); },
                 "Publish the staged product at its new destination. Destruction never publishes.");
    }

    // Writer calls retain the GIL while borrowing Python buffers and mutating the build.
    void bindDictionaryWriter(nb::module_& module)
    {
        nb::enum_<HeadlineRole>(module, "HeadlineRole")
            .value("PRIMARY", HeadlineRole::Primary)
            .value("SECONDARY", HeadlineRole::Secondary)
            .value("SHORT", HeadlineRole::Short);

        auto type = nb::class_<DictionaryWriter>(module, "DictionaryWriter",
                                                 "Build one dictionary serially. Inputs are consumed during each call.");
        type.def(
            "add_page", [](const DictionaryWriter& dictionary, uint32_t pageId, std::string_view xml)
            {
                unwrap(dictionary.addPage(pageId, xml));
            }, "page_id"_a, "xml"_a);

        type.def(
            "add_headline",
            [](const DictionaryWriter& dictionary, HeadlineComponents headline, HeadlineRole role)
            {
                unwrap(dictionary.addHeadline(std::move(headline), role));
            }, "headline"_a, nb::kw_only(),
            "role"_a = HeadlineRole::Primary);

        type.def(
            "add_search_key",
            [](const DictionaryWriter& dictionary, SearchScope scope, std::string_view key, EntryId target,
               bool exactOnly)
            {
                unwrap(dictionary.addSearchKey(scope, key, target, {.exactOnly = exactOnly}));
            },
            "scope"_a, "key"_a, "target"_a, nb::kw_only(), "exact_only"_a = false);

        type.def(
            "add_gaiji", [](const DictionaryWriter& dictionary, std::string_view filename, nb::handle data)
            {
                return unwrap(dictionary.addGaiji(filename, InputBytes(data).span()));
            }, "filename"_a,
            "data"_a);

        type.def(
            "add_graphic", [](const DictionaryWriter& dictionary, std::string_view name, nb::handle data)
            {
                unwrap(dictionary.addGraphic(name, InputBytes(data).span()));
            }, "name"_a, "data"_a);

        type.def(
            "add_audio", [](const DictionaryWriter& dictionary, std::string_view name, nb::handle data)
            {
                unwrap(dictionary.addAudio(name, InputBytes(data).span()));
            }, "name"_a, "data"_a);

        type.def(
            "add_asset", [](const DictionaryWriter& dictionary, std::string_view path, nb::handle data)
            {
                unwrap(dictionary.addAsset(path, InputBytes(data).span()));
            }, "path"_a, "data"_a);

        type.def(
            "add_font", [](const DictionaryWriter& dictionary, std::string_view filename, nb::handle data)
            {
                unwrap(dictionary.addFont(filename, InputBytes(data).span()));
            }, "filename"_a, "data"_a);

        type.def(
            "set_presentation", [](const DictionaryWriter& dictionary, DictionaryPresentation presentation)
            {
                unwrap(dictionary.setPresentation(std::move(presentation)));
            }, "presentation"_a);

        type.def(
            "set_entry_order", [](const DictionaryWriter& dictionary, const std::vector<uint32_t>& ids)
            {
                unwrap(dictionary.setEntryOrder(ids));
            }, "page_ids"_a);

        type.def(
            "add_appendix",
            [](const DictionaryWriter& dictionary, std::string_view path, std::string_view title)
            {
                unwrap(dictionary.addAppendix(path, title));
            }, "path"_a, "title"_a);

        type.def("add_appendix_entries", &addAppendixEntries, "name"_a, "title"_a, "entries"_a, nb::kw_only(),
                 "rows"_a = 0, "columns"_a = 1);

        type.def("finish", [](const DictionaryWriter& dictionary) { unwrap(dictionary.finish()); },
                 "Validate references and complete this dictionary before publishing its product.");
    }
}
