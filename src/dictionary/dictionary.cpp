//
// kiwakiwaaにより 2026/01/15 に作成されました。
//


#include "monokakido/dictionary/dictionary.hpp"
#include "monokakido/dictionary/catalog.hpp"
#include "monokakido/resource/resource_loader.hpp"

#include <iostream>
#include <utility>

namespace monokakido
{
    std::expected<Dictionary, std::string> Dictionary::open(std::string_view dictId)
    {
        const auto& catalog = DictionaryCatalog::instance();

        const auto dictInfo = catalog.findById(dictId);
        if (!dictInfo)
            return std::unexpected(std::format("Dictionary '{}' not found", dictId));

        return openAtPath(dictInfo->path);
    }


    std::expected<Dictionary, std::string> Dictionary::openAtPath(const fs::path& path)
    {
        auto dictId = path.stem().string();

        auto metadata = DictionaryMetadata::loadFromPath(
            path / "Contents" / (dictId + ".json")
        );
        if (!metadata)
            return std::unexpected(std::format("Failed to load dictionary metadata: '{}'", metadata.error()));

        auto paths = DictionaryPaths::create(path, metadata.value());
        if (!paths)
            return std::unexpected(std::format("Failed to load paths: '{}'", paths.error()));

        ResourceLoader loader(*paths);

        auto entries = loader.loadEntries();
        auto audio = loader.loadAudio();
        auto graphics = loader.loadGraphics();
        auto fonts = loader.loadFonts();

        return Dictionary(
            std::string(dictId),
            std::move(metadata.value()),
            std::move(paths.value()),
            std::move(entries),
            std::move(graphics),
            std::move(audio),
            std::move(fonts)
        );
    }


    const std::string& Dictionary::id() const noexcept
    {
        return id_;
    }


    const DictionaryMetadata& Dictionary::metadata() const noexcept
    {
        return metadata_;
    }


    Nrsc* Dictionary::graphics() noexcept
    {
        if (!graphics_ || graphics_->empty())
            return nullptr;

        return &*graphics_;
    }


    const Nrsc* Dictionary::graphics() const noexcept
    {
        if (!graphics_ || graphics_->empty())
            return nullptr;

        return &*graphics_;
    }


    bool Dictionary::hasGraphics() const noexcept
    {
        return graphics_ && !graphics_->empty();
    }


    bool Dictionary::hasFonts() const noexcept
    {
        return !fonts_.empty();
    }


    Dictionary::Dictionary(std::string id,
                           DictionaryMetadata metadata,
                           DictionaryPaths paths,
                           std::optional<Rsc> entries,
                           std::optional<Nrsc> graphics,
                           std::optional<Nrsc> audio,
                           std::vector<Font> fonts)
        : id_(std::move(id)), paths_(std::move(paths)), metadata_(std::move(metadata)),
          entries_(std::move(entries)),
          graphics_(std::move(graphics)),
          audio_(std::move(audio)),
          fonts_(std::move(fonts))
    {
    }


    void Dictionary::print() const
    {
        std::cout << "-------" << metadata_.displayName().value_or("[Display Name]") << "-----\n";
        std::cout << std::format("ID: {}", id_) << '\n';
        std::cout << std::format("Description: {}", metadata_.description().value_or("[Missing]")) << '\n';
        std::cout << std::format("Publisher: {}", metadata_.publisher().value_or("[Missing]")) << '\n';
        std::cout << std::format("Content dir: {}",
                                 metadata_.contentDirectoryName().value_or(fs::path("[Missing]")).string()) <<
                std::endl;
    }


    std::expected<ExportResult, std::string> Dictionary::exportEntries(const ExportOptions& options) const
    {
        if (!entries_)
            return ExportResult{};

        return RscExporter::exportAll(*entries_, options);
    }


    std::expected<ExportResult, std::string> Dictionary::exportGraphics(
        const ExportOptions& options) const
    {
        return hasGraphics() ? NrscExporter::exportAll(*graphics_, options) : ExportResult{};
    }


    std::expected<ExportResult, std::string> Dictionary::exportFonts(
        const ExportOptions& options) const
    {
        ExportResult combinedResult;
        for (auto& font : fonts_)
        {
            auto result = FontExporter::exportFont(font, options);
            if (!result)
                return result;

            combinedResult.totalResources += result->totalResources;
            combinedResult.exported += result->exported;
            combinedResult.skipped += result->skipped;
            combinedResult.failed += result->failed;
            combinedResult.totalBytes += result->totalBytes;
            combinedResult.errors.insert(combinedResult.errors.end(), result->errors.begin(), result->errors.end());
        }
        return combinedResult;
    }
}
