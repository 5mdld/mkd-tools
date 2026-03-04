//
// kiwakiwaaにより 2026/02/19 に作成されました。
//

#pragma once

#include "MKD/platform/dictionary_source.hpp"

namespace MKD
{
    static constexpr auto MONOKAKIDO_GROUP_ID = "group.jp.monokakido.Dictionaries";
    static constexpr auto DICTIONARIES_SUBPATH = "Library/Application Support/com.dictionarystore/dictionaries";

    class MacOSDictionarySource final : public DictionarySource
    {
    public:
        MacOSDictionarySource();
        ~MacOSDictionarySource() override;

        MacOSDictionarySource(MacOSDictionarySource&&) noexcept;
        MacOSDictionarySource& operator=(MacOSDictionarySource&&) noexcept;

        [[nodiscard]] Result<std::vector<DictionaryInfo>> findAllAvailable() const override;
        [[nodiscard]] Result<DictionaryInfo> findById(std::string_view dictId) const override;

        [[nodiscard]] bool checkAccess() const;
        [[nodiscard]] bool requestAccess(bool activateProcess = false) const;

        static bool isMonokakidoInstalled() ;

    private:
        [[nodiscard]] bool tryRestoreFromBookmark() const;

        [[nodiscard]] bool canAccessDirectly() const;

        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}