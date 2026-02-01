//
// kiwakiwaaにより 2026/01/18 に作成されました。
//

#pragma once

#include "monokakido/dictionary/paths.hpp"
#include "monokakido/resource/nrsc/nrsc.hpp"
#include "monokakido/resource/font.hpp"


namespace monokakido
{

    template<typename T>
    concept Openable = requires(const fs::path& path)
    {
        { T::open(path) } -> std::same_as<std::expected<T, std::string>>;
    };

    class ResourceLoader
    {
    public:

        explicit ResourceLoader(const DictionaryPaths& paths);

        [[nodiscard]] std::optional<Nrsc> loadGraphics();
        [[nodiscard]] std::optional<Nrsc> loadAudio();
        [[nodiscard]] std::vector<Font> loadFonts() const;

        // [[nodiscard]] std::optional<resource::Rsc> loadEntries();

    private:

        const DictionaryPaths& paths_;

        template<Openable T>
        std::optional<T> tryLoad(PathType pathType, std::string_view resourceName);
    };



}