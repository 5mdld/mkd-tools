//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/dictionary/product_writer.hpp"
#include "MKD/resource/resource_store_writer.hpp"
#include "MKD/resource/named_resource_store_writer.hpp"
#include "MKD/resource/headline_store_writer.hpp"
#include "io/build_directory.hpp"
#include "content_references.hpp"
#include "dictionary/package/native_format.hpp"
#include "output/directory_output.hpp"

#include <map>
#include <set>
#include <unordered_set>
#include <type_traits>

namespace MKD::detail
{
    struct DictionaryBuild;

    struct ProductBuild
    {
        std::unique_ptr<BuildDirectory> output;
        DictionaryMetadata metadata;
        std::string name;
        CreationProfile profile = CreationProfile::General;
        ProductInformation information;
        std::optional<AppendixMenu> appendices;
        std::optional<NavigationIndex> navigation;
        std::vector<AppendixEntryLink> appendixReferences;
        std::vector<std::shared_ptr<DictionaryBuild>> dictionaries;
        explicit ProductBuild(std::unique_ptr<BuildDirectory> output, DictionaryMetadata metadata, std::string name);
        Result<void> writeAsset(std::string_view relative, std::span<const uint8_t> bytes) const;
        Result<void> writeAppendices() const;
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
        struct PassageIndex
        {
            SearchScope scope;
            std::set<std::string> terms;
        };
        std::map<EntryId, PassageIndex> passageIndexes;
        std::optional<NamedResourceStoreWriter> graphics;
        std::optional<NamedResourceStoreWriter> audio;
        std::unordered_set<uint32_t> pageIds;
        std::map<uint32_t, std::array<uint32_t, native::entry::targetRanges.size()>> reservedEntries;
        uint32_t nextPageId = 1;
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

        std::vector<AppendixItem> appendices;
        bool finished = false;

        DictionaryBuild(std::shared_ptr<ProductBuild> product, DictionaryContent metadata,
                        DictionaryWriteOptions options);
        Result<std::shared_ptr<ProductBuild>> owner() const;
        Result<void> fail(std::string error) const;
        void recordReferences(const ContentReferences& references, std::string origin);
        Result<void> scanHeadline(const HeadlineComponents& headline);
        Result<void> scanStylesheet(std::string_view path, std::span<const uint8_t> bytes);
        Result<void> scanPage(uint32_t pageId, std::string_view xhtml);
        Result<uint32_t> reserveEntry(std::optional<uint32_t> pageId);
        Result<EntryId> addTarget(uint32_t pageId, EntryTargetKind kind);
        Result<void> writePage(uint32_t pageId, std::string_view xml);
        Result<void> addSearchKey(SearchScope scope, std::string_view key, EntryId target, SearchKeyOptions options);
        Result<void> writePassageHeadlines();
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
        auto write(Operation&& operation) -> std::invoke_result_t<Operation, detail::DictionaryBuild&>
        {
            using Return = std::invoke_result_t<Operation, detail::DictionaryBuild&>;
            auto product = dictionary->owner();
            if (!product)
                return Return(std::unexpected(product.error()));

            auto result = operation(*dictionary);
            if (!result)
                (void)(*product)->output->fail(result.error());

            return result;
        }
    };

    struct EntryWriter::Impl
    {
        DictionaryWriter::Impl writer;
        uint32_t pageId;
    };
}
