//
// kiwakiwaaにより 2026/02/19 に作成されました。
//

#include "MKD/platform/directory_dictionary_source.hpp"

#include <array>
#include <format>

namespace MKD
{
    DirectoryDictionarySource::DirectoryDictionarySource(fs::path root)
        : root_(std::move(root))
    {
    }

    Result<void> DirectoryDictionarySource::validateDirectory() const
    {
        std::error_code error;
        const auto status = fs::status(root_, error);
        if (error == std::errc::no_such_file_or_directory || (!error && !fs::exists(status)))
            return std::unexpected(std::format("Dictionary directory does not exist: {}", root_.string()));

        if (error)
            return std::unexpected(std::format("Cannot inspect dictionary directory '{}': {}", root_.string(),
                                               error.message()));

        if (!fs::is_directory(status))
            return std::unexpected(std::format("Not a directory: {}", root_.string()));

        return {};
    }

    Result<std::vector<DictionaryInfo>> DirectoryDictionarySource::findAllAvailable() const
    {
        if (auto valid = validateDirectory(); !valid)
            return std::unexpected(valid.error());

        std::vector<DictionaryInfo> result;
        std::error_code error;
        fs::directory_iterator iterator(root_, error), end;
        while (!error && iterator != end)
        {
            const auto& entry = *iterator;
            const bool directory = entry.is_directory(error);
            if (error)
                return std::unexpected(std::format("Cannot inspect '{}': {}", entry.path().string(), error.message()));
            if (directory)
            {
                auto valid = looksLikeDictionary(entry.path());
                if (!valid)
                    return std::unexpected(valid.error());
                if (*valid)
                    result.emplace_back(entry.path().filename().string(), entry.path());
            }
            iterator.increment(error);
        }
        if (error)
            return std::unexpected(std::format("Cannot read dictionary directory '{}': {}", root_.string(),
                                               error.message()));
        return result;
    }

    Result<DictionaryInfo> DirectoryDictionarySource::findById(std::string_view dictId) const
    {
        if (auto valid = validateDirectory(); !valid)
            return std::unexpected(valid.error());

        const auto path = root_ / dictId;
        std::error_code error;
        const auto status = fs::status(path, error);
        if (error == std::errc::no_such_file_or_directory || (!error && !fs::exists(status)))
            return std::unexpected(std::format("Dictionary '{}' not found in {}", dictId, root_.string()));
        if (error)
            return std::unexpected(std::format("Cannot inspect '{}': {}", path.string(), error.message()));
        if (!fs::is_directory(status))
            return std::unexpected(std::format("'{}' is not a directory", path.string()));

        auto valid = looksLikeDictionary(path);
        if (!valid)
            return std::unexpected(valid.error());
        if (!*valid)
            return std::unexpected(std::format("'{}' does not appear to be a valid dictionary", dictId));
        return DictionaryInfo{std::string(dictId), path};
    }

    Result<bool> DirectoryDictionarySource::looksLikeDictionary(const fs::path& path)
    {
        const auto contents = path / "Contents";
        const std::array files = {path / "ContentInfo.plist", contents / (path.filename().string() + ".json")};
        for (const auto& file : files)
        {
            std::error_code error;
            const bool regular = fs::is_regular_file(file, error);
            if (error == std::errc::no_such_file_or_directory || error == std::errc::not_a_directory)
                return false;
            if (error)
                return std::unexpected(std::format("Cannot inspect '{}': {}", file.string(), error.message()));
            if (!regular)
                return false;
        }
        return true;
    }
}
