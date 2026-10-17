//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "io/output_file.hpp"
#include "MKD/resource/appendix_entry_list.hpp"
#include "MKD/resource/headline_store.hpp"
#include "MKD/resource/keystore.hpp"

namespace MKD::detail
{
    Result<void> writePrettyXml(std::span<const uint8_t> bytes, OutputFile& output);
    Result<void> writeHeadlines(const HeadlineStore& store, OutputFile& output);
    Result<void> writeForwardKeys(const Keystore& store, OutputFile& output);
    Result<void> writeInverseKeys(const Keystore& store, OutputFile& output);
    Result<void> writeAppendixEntries(const AppendixEntryList& list,
                                      std::span<const HeadlineStore> headlines, OutputFile& output);
}
