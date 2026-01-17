//
// Caoimheにより 2026/01/15 に作成されました。
//

#pragma once

#include "metadata.hpp"
#include "paths.hpp"
#include "monokakido/resource/nrsc/nrsc.hpp"
#include "monokakido/resource/export/exporter.hpp"

namespace monokakido::dictionary
{

    class Dictionary
    {
    public:

        static std::expected<Dictionary, std::string> open(std::string_view dictId);

        //static std::expected<Dictionary, std::string> openAtPath(const fs::path& path);

        void print() const;

        void exportAllResources() const;


    private:

        Dictionary(std::string id, DictionaryMetadata metadata, DictionaryPaths paths);

        std::string id_;
        DictionaryPaths paths_;
        DictionaryMetadata metadata_;

    };

}