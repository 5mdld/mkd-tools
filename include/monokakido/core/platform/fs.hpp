//
// Caoimheにより 2026/01/14 に作成されました。
//

#pragma once

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace monokakido::platform::fs
{
    struct BookmarkData
    {
        std::vector<uint8_t> data;
        std::filesystem::path resolvedPath;
    };

    [[nodiscard]] std::filesystem::path getContainerPathByGroupIdentifier(const std::string& groupIdentifier);

    [[nodiscard]] std::expected<std::string, std::error_code> readTextFile(const std::filesystem::path& path);

    [[nodiscard]] std::string makeFilestreamError(const std::ifstream& file, std::string_view context);

    std::optional<BookmarkData> promptForDictionariesAccess();

    std::optional<std::vector<uint8_t>> loadSavedBookmark();

    void saveBookmark(const std::vector<uint8_t>& bookmarkData);

    void clearSavedBookmark();
}
