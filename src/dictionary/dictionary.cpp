//
// Caoimheにより 2026/01/15 に作成されました。
//


#include "monokakido/dictionary/dictionary.hpp"
#include "monokakido/dictionary/catalog.hpp"

#include <iostream>
#include <utility>

namespace monokakido::dictionary
{
    std::expected<Dictionary, std::string> Dictionary::open(std::string_view dictId)
    {
        auto& catalog = DictionaryCatalog::instance();

        auto dictInfo = catalog.findById(dictId);
        if (!dictInfo)
            return std::unexpected(std::format("Dictionary '{}' not found", dictId));

        auto metadata = DictionaryMetadata::loadFromPath(
            dictInfo->path / "Contents" / (dictInfo->id + ".json")
        );
        if (!metadata)
            return std::unexpected(std::format("Failed to load metadata: '{}'", metadata.error()));

        auto paths = DictionaryPaths::create(dictInfo->path, metadata.value());
        if (!paths)
            return std::unexpected(std::format("Failed to load paths: '{}'", paths.error()));

        return Dictionary(
            std::string(dictId),
            std::move(metadata.value()),
            std::move(paths.value())
        );
    }


    Dictionary::Dictionary(std::string id, DictionaryMetadata metadata, DictionaryPaths paths)
        : id_(std::move(id)), paths_(std::move(paths)), metadata_(std::move(metadata))
    {
    }


    void Dictionary::print() const
    {
        std::cout << "-------" << metadata_.displayName().value_or("[Display Name]") << "-----\n";
        std::cout << std::format("ID: {}", id_) << '\n';
        std::cout << std::format("Description: {}", metadata_.description().value_or("[Missing]")) << '\n';
        std::cout << std::format("Publisher: {}", metadata_.publisher().value_or("[Missing]")) << '\n';
        std::cout << std::format("Content dir: {}",
                                 metadata_.contentDirectoryName().value_or(fs::path("[Missing]")).string()) << std::endl;
    }


    void Dictionary::exportAllResources() const
    {
        auto nrsc = resource::Nrsc::open(paths_.resolve(PathType::Graphics));
        if (!nrsc)
        {
            std::cerr << nrsc.error() << std::endl;
            return;
        }

        auto exporter = resource::ResourceExporter({.outputDirectory = fs::path(std::getenv("HOME")) / "Downloads/out"});
        auto result = exporter.exportAll(nrsc.value());
    }
}
