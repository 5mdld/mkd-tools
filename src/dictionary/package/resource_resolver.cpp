//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "dictionary/package/resource_resolver.hpp"
#include "dictionary/package/package_format.hpp"
#include "io/write_validation.hpp"


namespace MKD::detail
{
    Result<RetainedSpan> readDictionaryResource(const std::filesystem::path& directory,
                                                const NamedResourceStore* graphics,
                                                const std::variant<ResourceStore, NamedResourceStore>* audio,
                                                std::string_view path)
    {
        if (auto valid = WriteValidation::relativePath(path); !valid)
            return std::unexpected(valid.error());

        if (path.find_first_of("?#") != std::string_view::npos)
            return std::unexpected("Resource lookup expects a path without a query or fragment");

        if (!directory.empty())
        {
            const auto file = directory / WriteValidation::fromUtf8(path);
            std::error_code error;
            if (std::filesystem::is_regular_file(file, error))
                return RetainedSpan::map(file);

            if (error && error != std::errc::no_such_file_or_directory)
                return std::unexpected(error.message());
        }

        const auto slash = path.find('/');
        const auto storeName = path.substr(0, slash);
        const auto name = slash == std::string_view::npos ? std::string_view{} : path.substr(slash + 1);
        if (!name.empty() && (storeName == monokakido::paths::graphics || storeName == monokakido::paths::legacyGraphics))
        {
            bool useLoaded = directory.empty();
            if (!directory.empty())
            {
                std::error_code error;
                const auto preferred = std::filesystem::is_directory(directory / monokakido::paths::graphics, error)
                                           ? monokakido::paths::graphics
                                           : monokakido::paths::legacyGraphics;
                useLoaded = storeName == preferred;
            }

            if (graphics && useLoaded)
                return graphics->get(name);

            if (!directory.empty())
            {
                auto store = NamedResourceStore::open(directory / WriteValidation::fromUtf8(storeName));
                if (!store)
                    return std::unexpected(store.error());
                return store->get(name);
            }
        }
        if (audio && storeName == monokakido::paths::audio && !name.empty())
            if (const auto store = std::get_if<NamedResourceStore>(audio))
                return store->get(name);

        return std::unexpected("Missing dictionary resource: " + std::string(path));
    }
}
