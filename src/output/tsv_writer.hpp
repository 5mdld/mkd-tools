//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "io/output_file.hpp"
#include "MKD/resource/entry_id.hpp"

namespace MKD::detail
{
    class TsvWriter
    {
    public:
        explicit TsvWriter(OutputFile& output);
        void field(std::string_view value);
        void field(std::u16string_view value);
        void field(EntryId value);
        Result<void> endRow();
        Result<void> finish();

    private:
        OutputFile& output_;
        std::string buffer_;
        bool firstField_ = true;
        std::optional<std::string> error_;
    };
}
