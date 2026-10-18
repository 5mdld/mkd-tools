//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "support/test_dictionary.hpp"
#include <fstream>
#include <random>

namespace MKD::test
{
    std::filesystem::path dictionaries() { return MKD_TEST_DICTIONARIES; }
    std::filesystem::path assets() { return MKD_TEST_ASSETS; }

    std::span<const uint8_t> bytes(const std::string_view text)
    {
        return {reinterpret_cast<const uint8_t*>(text.data()), text.size()};
    }

    std::vector<uint8_t> readFile(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("Cannot read " + path.string());
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }

    void writeFile(const std::filesystem::path& path, const std::span<const uint8_t> data)
    {
        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        if (!file)
            throw std::runtime_error("Cannot write " + path.string());
    }

    Workspace::Workspace()
        : path(std::filesystem::temp_directory_path() / ("mkd-birbs-" + std::to_string(std::random_device{}())))
    {
        if (!std::filesystem::create_directory(path))
            throw std::runtime_error("Cannot create test workspace " + path.string());
    }

    Workspace::~Workspace()
    {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }

    DictionaryContent birbContent()
    {
        return {
            .identifier = "test.birbs", .directory = "birbs", .dictionaryId = 1000,
            .title = LocalizedString{.ja = "小鳥たち"}, .languages = std::vector<std::string>{"ja", "en"}
        };
    }

    BirbBuild::BirbBuild(const std::filesystem::path& path, const bool encrypted)
        : product(require(ProductWriter::create(path, {.displayName = LocalizedString{.en = "Birbs"}}))),
          dictionary(require(product.addDictionary(birbContent(), {.encryptContents = encrypted})))
    {
    }

    EntryId BirbBuild::addBirb(const std::optional<uint32_t> pageId)
    {
        auto entry = require(dictionary.createEntry(pageId));
        const auto target = entry.target();
        require(entry.writeXml("<html><body><h1>雀</h1><p>A small birb.</p></body></html>"));
        require(dictionary.addHeadline({.headline = u"雀", .entryId = target}));
        require(dictionary.addSearchKey(SearchScope::Headword, "すずめ", target));
        return target;
    }

    void BirbBuild::finish()
    {
        require(dictionary.finish());
        require(product.finish());
    }
}
