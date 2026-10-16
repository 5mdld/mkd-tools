//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#pragma once

#include "MKDCLI/cli.hpp"
#include "MKD/platform/dictionary_source.hpp"

#include <memory>

namespace MKDCLI
{
    MKD::Result<std::unique_ptr<MKD::DictionarySource>> createSource(const GlobalOptions& options);
}
