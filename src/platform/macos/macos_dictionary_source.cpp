//
// kiwakiwaaにより 2026/02/19 に作成されました。
//

#include "MKD/platform/macos/macos_dictionary_source.hpp"
#include "MKD/platform/macos/fs.hpp"

#include <format>

namespace MKD
{
    std::expected<std::vector<DictionaryInfo>, std::string> MacOSDictionarySource::findAllAvailable() const
    {
        if (!checkAccess())
            return std::unexpected("Access to dictionaries folder not granted. Please call requestAccess().");

        if (!authorizedPath_)
            return std::unexpected("No authorized path available");

        std::vector<DictionaryInfo> result;
        const auto containerPath = macOS::getContainerPathByGroupIdentifier(MONOKAKIDO_GROUP_ID);
        if (containerPath.empty())
            return result;

        try
        {
            for (const auto path = containerPath / DICTIONARIES_PATH; const auto& folder : fs::directory_iterator(path))
            {
                if (!folder.is_directory())
                    continue;

                const auto info = DictionaryInfo(folder.path().stem(), folder.path());
                result.emplace_back(info);
            }
        }
        catch (const fs::filesystem_error& e)
        {
            return std::unexpected(std::format("Failed to read dictionaries: {}", e.what()));
        }

        cachedDictionaries_ = result;
        return result;
    }


    std::expected<DictionaryInfo, std::string> MacOSDictionarySource::findById(std::string_view dictId) const
    {
        if (!checkAccess())
            return std::unexpected("Access to dictionaries folder not granted. Please call requestAccess().");

        if (!authorizedPath_)
            return std::unexpected("No authorized path available");

        if (!cachedDictionaries_)
            findAllAvailable();

        for (const auto& info : cachedDictionaries_.value())
        {
            if (info.id == dictId)
                return info;
        }

        return std::unexpected(std::format("Failed to find dictionary with id '{}'", dictId));
    }


    bool MacOSDictionarySource::checkAccess() const
    {
        if (securityAccess_.isValid())
            return true;

        // Attempt to restore access from saved bookmark
        if (const auto bookmark = macOS::loadSavedBookmark())
        {
            if (auto access = macOS::restoreAccessFromBookmark(*bookmark))
            {
                authorizedPath_ = access->path;
                securityAccess_ = std::move(access->access);
                return true;
            }
        }

        return false;
    }


    bool MacOSDictionarySource::requestAccess() const
    {
        auto bookmarkData = macOS::promptForDictionariesAccess();
        if (!bookmarkData)
            return false;

        macOS::saveBookmark(bookmarkData->data);

        authorizedPath_ = bookmarkData->resolvedPath;
        securityAccess_ = macOS::ScopedSecurityAccess(bookmarkData->resolvedPath);

        if (!securityAccess_.isValid())
        {
            authorizedPath_.reset();
            return false;
        }

        // Clear cache to force refresh
        cachedDictionaries_.reset();
        return true;
    }
}
