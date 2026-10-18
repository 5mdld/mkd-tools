//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include <filesystem>
#include <string_view>

namespace MKDCLI::macOS
{
    class InstallTransaction
    {
    public:
        explicit InstallTransaction(std::filesystem::path store);
        ~InstallTransaction();
        InstallTransaction(const InstallTransaction&) = delete;
        InstallTransaction& operator=(const InstallTransaction&) = delete;

        MKD::Result<void> begin(std::string_view name);
        const std::filesystem::path& stagedPackage() const { return staged_; }
        MKD::Result<void> publish(bool replace);
        MKD::Result<void> complete();
        std::unexpected<std::string> fail(std::string error);

    private:
        std::filesystem::path store_;
        std::filesystem::path staged_;
        std::filesystem::path destination_;
        int lock_ = -1;
        bool active_ = false;
        bool published_ = false;
    };
}
