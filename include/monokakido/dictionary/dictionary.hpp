//
// kiwakiwaaにより 2026/01/15 に作成されました。
//

#pragma once

#include "metadata.hpp"
#include "paths.hpp"
#include "monokakido/resource/nrsc/nrsc.hpp"
#include "monokakido/output/exporter.hpp"
#include "monokakido/resource/font.hpp"

namespace monokakido
{

    class Dictionary
    {
    public:

        static std::expected<Dictionary, std::string> open(std::string_view dictId);

        static std::expected<Dictionary, std::string> openAtPath(const fs::path& path);

        [[nodiscard]] const std::string& id() const noexcept;
        [[nodiscard]] const DictionaryMetadata& metadata() const noexcept;

        // returns nullptr if no nrsc resources available
        [[nodiscard]] Nrsc* graphics() noexcept;
        [[nodiscard]] const Nrsc* graphics() const noexcept;
        [[nodiscard]] Rsc* fonts() noexcept;
        [[nodiscard]] const Rsc* fonts() const noexcept;

        [[nodiscard]] bool hasGraphics() const noexcept;
        [[nodiscard]] bool hasFonts() const noexcept;


        std::expected<ExportResult, std::string> exportAllResources(const ExportOptions& options) const;

        std::expected<ExportResult, std::string> exportGraphics(const ExportOptions& options) const;
        std::expected<ExportResult, std::string> exportFonts(const ExportOptions& options) const;

        void print() const;


    private:

        Dictionary(
            std::string id,
            DictionaryMetadata metadata,
            DictionaryPaths paths,
            std::optional<Nrsc> graphics,
            std::optional<Nrsc> audio,
            std::vector<Font> fonts);

        std::string id_;
        DictionaryPaths paths_;
        DictionaryMetadata metadata_;

        std::optional<Nrsc> graphics_;
        std::optional<Nrsc> audio_;
        // Rsc entryContent_; // xml
        std::vector<Font> fonts_;

    };
}