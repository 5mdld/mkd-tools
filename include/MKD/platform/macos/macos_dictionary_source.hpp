//
// kiwakiwaaにより 2026/02/19 に作成されました。
//

#pragma once

#include "MKD/platform/dictionary_source.hpp"
#include "scoped_security_access.hpp"

#include <optional>

namespace MKD
{
    static constexpr auto MONOKAKIDO_GROUP_ID = "group.jp.monokakido.Dictionaries";
    static constexpr auto DICTIONARIES_PATH = "Library/Application Support/com.dictionarystore/dictionaries";

    class MacOSDictionarySource final : public DictionarySource
    {
    public:

        std::expected<std::vector<DictionaryInfo>, std::string> findAllAvailable() const override;

        [[nodiscard]] std::expected<DictionaryInfo, std::string> findById(std::string_view dictId) const override;

        enum class AccessStatus
        {
            Granted,
            NeedsPermission,
            PermissionDenied
        };

        AccessStatus checkAccess() const;
        bool requestAccess() const;

    private:
        mutable std::optional<std::vector<DictionaryInfo>> cachedDictionaries_;
        mutable std::optional<fs::path> authorizedPath_;
        mutable macOS::ScopedSecurityAccess securityAccess_;

    };
}