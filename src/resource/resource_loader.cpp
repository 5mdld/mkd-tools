//
// kiwakiwaaにより 2026/01/18 に作成されました。
//

#include "monokakido/resource/resource_loader.hpp"

#include <iostream>

namespace monokakido
{
    ResourceLoader::ResourceLoader(const DictionaryPaths& paths)
        : paths_(paths)
    {
    }


    std::optional<Rsc> ResourceLoader::loadEntries()
    {
        return tryLoad<Rsc>(PathType::Contents, "contents");
    }


    std::optional<Nrsc> ResourceLoader::loadGraphics()
    {
        return tryLoad<Nrsc>(PathType::Graphics, "graphics");
    }


    std::optional<Nrsc> ResourceLoader::loadAudio()
    {
        return tryLoad<Nrsc>(PathType::Audio, "audio");
    }


    std::vector<Font> ResourceLoader::loadFonts() const
    {
        const auto fontsPath = paths_.tryResolve(PathType::Fonts);
        if (!fontsPath)
            return {};

        std::vector<Font> fonts;

        // Iterate through subdirectories in the fonts folder
        for (const auto& entry : fs::directory_iterator(*fontsPath))
        {
            if (!entry.is_directory())
                continue;

            const auto fontName = entry.path().filename().string();
            const auto& fontPath = entry.path();

            const bool hasRscFile = std::ranges::any_of(
                fs::directory_iterator(fontPath),
                [](const auto& fileEntry) {
                    return fileEntry.path().extension() == ".rsc";
                }
            );

            if (!hasRscFile)
                continue;

            // Try to open the Rsc from this font directory
            auto result = Rsc::open(fontPath);
            if (!result)
            {
                std::cerr << std::format("Failed to open font '{}' at: {}\n{}",
                                       fontName, fontPath.string(), result.error());
                continue;
            }

            fonts.emplace_back(fontName, std::move(*result));
        }

        return fonts;
    }


    template<Openable T>
    std::optional<T> ResourceLoader::tryLoad(const PathType pathType, std::string_view resourceName)
    {
        const auto path = paths_.tryResolve(pathType);
        if (!path)
            return std::nullopt;

        const bool hasResource = std::ranges::any_of(
            fs::directory_iterator(*path),
            [](const auto& entry) {
                const auto& p = entry.path();
                return p.extension() == ".rsc" || p.extension() == ".nrsc";
            }
        );

        if (!hasResource)
            return std::nullopt;

        auto result = T::open(*path);
        if (!result)
        {
            std::cerr << std::format("Failed to open {} at: {}\n{}",
                                     resourceName, path->string(), result.error());
            return std::nullopt;
        }

        return std::move(*result);
    }
}
