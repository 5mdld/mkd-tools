//
// kiwakiwaaにより 2026/01/18 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include "MKD/dictionary/paths.hpp"
#include "MKD/resource/font.hpp"
#include "MKD/resource/named_resource_store.hpp"
#include "MKD/resource/resource_store.hpp"
#include "MKD/resource/keystore.hpp"
#include "MKD/resource/headline_store.hpp"

#include <variant>

namespace MKD
{

    template<typename T>
    concept Openable = requires(const fs::path& path) {
        { T::open(path) } -> std::same_as<Result<T>>;
    } || requires(const fs::path& path, const std::string& dictId) {
        { T::open(path, dictId) } -> std::same_as<Result<T>>;
    };

    class ResourceLoader
    {
    public:

        explicit ResourceLoader(const DictionaryPaths& paths);

        [[nodiscard]] std::optional<ResourceStore> loadEntries(std::string_view contentDir, std::string_view dictId) const;
        [[nodiscard]] std::optional<NamedResourceStore> loadGraphics(std::string_view contentDir, std::string_view dictId) const;
        [[nodiscard]] std::optional<std::variant<ResourceStore, NamedResourceStore>> loadAudio(std::string_view contentDir, std::string_view dictId) const;
        [[nodiscard]] std::vector<Keystore> loadKeystores(std::string_view contentDir, std::string_view dictId) const;
        [[nodiscard]] std::vector<HeadlineStore> loadHeadlines(std::string_view contentDir) const;
        [[nodiscard]] std::vector<Font> loadFonts(std::string_view contentDir) const;

    private:

        const DictionaryPaths& paths_;

        template<Openable T>
        std::optional<T> tryLoad(ResourceType pathType, std::string_view contentDir, std::string_view dictId) const;

        template<typename T, typename Predicate, typename Loader>
        std::vector<T> loadCollection(ResourceType type, std::string_view contentDir, Predicate pred, Loader loader) const;

        [[nodiscard]] std::optional<std::variant<ResourceStore, NamedResourceStore>> tryLoadEither(ResourceType pathType, std::string_view contentDir, std::string_view dictId) const;

        static bool hasResourceFiles(const fs::path& path);
    };
}