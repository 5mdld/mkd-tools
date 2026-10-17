//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

// Copyright (c) 2026 kiwakiwaa
// https://github.com/kiwakiwaa
// SPDX-License-Identifier: GPL-3.0-only
// Licensed under the GNU General Public License v3.0.

#pragma once

#include "NSObject.hpp"

namespace NS
{
    class String;

    class Orthography : public Referencing<Orthography>
    {
    public:
        static Orthography* defaultOrthographyForLanguage(const String* pLanguage);
    };
}

_NS_INLINE NS::Orthography* NS::Orthography::defaultOrthographyForLanguage(const String* pLanguage)
{
    return Object::sendMessage<Orthography*>(_NS_PRIVATE_CLS(NSOrthography), _NS_PRIVATE_SEL(defaultOrthographyForLanguage_), pLanguage);
}
