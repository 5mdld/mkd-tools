//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "content_resources.hpp"
#include "native_content.hpp"
#include "native_format.hpp"
#include "io/write_validation.hpp"

#include <algorithm>
#include <array>
#include <pugixml.hpp>

namespace MKD::detail
{
    Result<RetainedSpan> ContentResources::read(const std::filesystem::path& directory,
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
        if (!name.empty() && (storeName == native::paths::graphics || storeName == native::paths::legacyGraphics))
        {
            bool useLoaded = directory.empty();
            if (!directory.empty())
            {
                std::error_code error;
                const auto preferred = std::filesystem::is_directory(directory / native::paths::graphics, error)
                                           ? native::paths::graphics
                                           : native::paths::legacyGraphics;
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
        if (audio && storeName == native::paths::audio && !name.empty())
            if (const auto store = std::get_if<NamedResourceStore>(audio))
                return store->get(name);

        return std::unexpected("Missing dictionary resource: " + std::string(path));
    }

    Result<void> ContentResources::validateGaiji(std::string_view path, std::span<const uint8_t> bytes)
    {
        if (auto valid = NativeContent::gaijiPath(path); !valid)
            return valid;

        if (path.ends_with(".svg"))
        {
            pugi::xml_document document;
            if (!document.load_buffer(bytes.data(), bytes.size(), pugi::parse_default, pugi::encoding_utf8) ||
                std::string_view(document.document_element().name()) != "svg")
                return std::unexpected("Gaiji SVG must be a well-formed SVG document");

            for (const auto node : document.children())
                if (node.type() == pugi::node_element && node != document.document_element())
                    return std::unexpected("Gaiji SVG must contain exactly one document element");
        }
        else
        {
            constexpr std::array<uint8_t, 8> signature{137, 80, 78, 71, 13, 10, 26, 10};
            if (bytes.size() < signature.size() || !std::ranges::equal(bytes.first(signature.size()), signature))
                return std::unexpected("Gaiji PNG has an invalid signature");
        }
        return {};
    }
}
