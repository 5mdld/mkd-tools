// Copyright (c) 2026 kiwakiwaa
// https://github.com/kiwakiwaa
// SPDX-License-Identifier: GPL-3.0-only
// Licensed under the GNU General Public License v3.0.

#pragma once

#include "NSObject.hpp"
#include "NSTypes.hpp"

namespace NS
{
    enum StringEncoding : UInteger
    {
        ASCIIStringEncoding = 1,
        UTF8StringEncoding = 4,
    };

    class String : public Copying<String>
    {
    public:
        static String* stringWithUTF8String(const char* pString);

        static String* alloc();
        String* initWithBytes(const void* pBytes, UInteger length, StringEncoding encoding);

        const char* utf8String() const;
    };
}

_NS_INLINE NS::String* NS::String::stringWithUTF8String(const char* pString)
{
    return Object::sendMessage<String*>(_NS_PRIVATE_CLS(NSString), _NS_PRIVATE_SEL(stringWithUTF8String_), pString);
}

_NS_INLINE NS::String* NS::String::alloc()
{
    return Object::alloc<String>(_NS_PRIVATE_CLS(NSString));
}

_NS_INLINE NS::String* NS::String::initWithBytes(const void* pBytes, const UInteger length, const StringEncoding encoding)
{
    return Object::sendMessage<String*>(this, _NS_PRIVATE_SEL(initWithBytes_length_encoding_), pBytes, length, encoding);
}

_NS_INLINE const char* NS::String::utf8String() const
{
    return Object::sendMessage<const char*>(this, _NS_PRIVATE_SEL(UTF8String));
}
