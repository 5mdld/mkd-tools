//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/dictionary/dictionary_writer.hpp"
#include "MKD/resource/resource_store_writer.hpp"
#include "MKD/resource/named_resource_store_writer.hpp"
#include "MKD/resource/headline_store_writer.hpp"
#include "io/build_directory.hpp"
#include "content_references.hpp"
#include "output/directory_output.hpp"

#include <map>
#include <set>
#include <unordered_set>

namespace MKD::detail
{
    struct DictionaryBuild;

    struct ProductBuild
    {
        std::unique_ptr<BuildDirectory> output;
        DictionaryMetadata metadata;
        std::string name;
        std::vector<std::shared_ptr<DictionaryBuild>> dictionaries;
        explicit ProductBuild(std::unique_ptr<BuildDirectory> output, DictionaryMetadata metadata, std::string name);
        Result<void> writeAsset(std::string_view relative, std::span<const uint8_t> bytes);
        Result<void> writeAppendices();
    };

    struct DictionaryBuild
    {
        std::weak_ptr<ProductBuild> product;
        DictionaryContent metadata;
        DictionaryWriteOptions options;
        std::filesystem::path root;
        std::optional<ResourceStoreWriter> pages;
        std::map<HeadlineRole, HeadlineStoreWriter> headlines;
        std::map<SearchScope, KeystoreWriter> keys;
        std::optional<NamedResourceStoreWriter> graphics;
        std::optional<NamedResourceStoreWriter> audio;
        std::unordered_set<uint32_t> pageIds;
        std::unordered_set<EntryId> anchors;
        std::unordered_set<EntryId> references;
        std::unordered_set<EntryId> primaryHeadlines;
        std::map<EntryId, std::string> targetOrigins;
        std::set<std::string> assetReferences;
        std::set<std::string> assets;
        std::set<std::string> gaijiReferences;
        std::map<std::string, std::string> assetOrigins;
        std::set<std::string> graphicNames;
        std::set<std::string> audioNames;
        std::vector<uint32_t> order;

        struct Appendix
        {
            std::string path;
            std::string title;
        };

        std::vector<Appendix> appendices;
        bool finished = false;

        DictionaryBuild(std::shared_ptr<ProductBuild> product, DictionaryContent metadata,
                        DictionaryWriteOptions options);
        Result<std::shared_ptr<ProductBuild>> owner() const;
        Result<void> fail(std::string error) const;
        void recordReferences(const ContentReferences& references, std::string origin);
        Result<void> scanHeadline(const HeadlineComponents& headline);
        Result<void> scanStylesheet(std::string_view path, std::span<const uint8_t> bytes);
        Result<void> scanPage(uint32_t pageId, std::string_view xhtml);
        Result<void> validateReferences() const;
        Result<void> validateEntryReferences() const;
        Result<void> validateAssetReferences() const;
        Result<void> validateOrder() const;
        Result<void> writeOrder();
        Result<void> writeAsset(std::string_view relative, std::span<const uint8_t> bytes);
        Result<void> writeMedia(std::string_view name, std::span<const uint8_t> bytes, bool isAudio);
        Result<void> finish();
    };
}

namespace MKD
{
    struct DictionaryWriter::Impl
    {
        std::shared_ptr<detail::DictionaryBuild> dictionary;

        explicit Impl(std::shared_ptr<detail::DictionaryBuild> dictionary) : dictionary(std::move(dictionary))
        {
        }

        template <typename Operation>
        Result<void> write(Operation&& operation)
        {
            auto product = dictionary->owner();
            if (!product) return std::unexpected(product.error());
            auto result = operation(*dictionary);
            if (!result) return (*product)->output->fail(result.error());
            return result;
        }
    };
}
