//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

// Copyright (c) 2026 kiwakiwaa
// https://github.com/kiwakiwaa
// SPDX-License-Identifier: GPL-3.0-only
// Licensed under the GNU General Public License v3.0.

#pragma once

#include "NSTypes.hpp"

namespace NS
{
    class String;

    using LinguisticTagScheme = const String*;
    using LinguisticTaggerOptions = UInteger;

    enum : LinguisticTaggerOptions
    {
        LinguisticTaggerOmitWhitespace = 1 << 2
    };

    namespace Private
    {
        extern "C" String* const NSLinguisticTagSchemeLemma;
    }

    inline const LinguisticTagScheme LinguisticTagSchemeLemma = Private::NSLinguisticTagSchemeLemma;
}
