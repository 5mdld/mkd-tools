// Copyright (c) 2026 kiwakiwaa
// https://github.com/kiwakiwaa
// SPDX-License-Identifier: GPL-3.0-only
// Licensed under the GNU General Public License v3.0.

#pragma once

#include "NSObject.hpp"
#include "NSTypes.hpp"

namespace NS
{
    class String : public Referencing<String>
    {
    public:
        static String* stringWithUTF8String(const char* pString);

        const char* utf8String() const;
    };
}

_NS_INLINE NS::String* NS::String::stringWithUTF8String(const char* pString)
{
    return Object::sendMessage<String*>(_NS_PRIVATE_CLS(NSString), _NS_PRIVATE_SEL(stringWithUTF8String_), pString);
}

_NS_INLINE const char* NS::String::utf8String() const
{
    return Object::sendMessage<const char*>(this, _NS_PRIVATE_SEL(UTF8String));
}
