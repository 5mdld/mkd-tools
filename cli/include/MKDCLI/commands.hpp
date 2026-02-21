//
// kiwakiwaaにより 2026/02/20 に作成されました。
//

#pragma once

#include "cli.hpp"
#include "MKD/platform/dictionary_source.hpp"

namespace MKDCLI
{
    /**
     * List all available dictionaries with metadata
     * @param source Dictionary source for dictionary discovery
     * @param opts Parsed CLI options
     * @return 0 if success, 1 if failure
     */
    int runList(MKD::DictionarySource& source, const CLIOptions& opts);

    /**
     * Print information about a single dictionary
     * @param source Dictionary source to print information about
     * @param opts Parsed CLI options
     * @return 0 if success, 1 if failure
     */
    int runInfo(const MKD::DictionarySource& source, const CLIOptions& opts);

    /**
     * Export unpacked dictionaries to disk
     * @param source Dictionary source to export
     * @param opts Parsed CLI options
     * @param exportOpts ExportOptions
     * @return 0 if success, 1 if failure
     */
    int runExport(const MKD::DictionarySource& source, const CLIOptions& opts, const MKD::ExportOptions& exportOpts);
}
