// Copyright (c) 2026 kiwakiwaa
// https://github.com/kiwakiwaa
// SPDX-License-Identifier: GPL-3.0-only
// Licensed under the GNU General Public License v3.0.

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
//
// Foundation/NSData.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "NSObject.hpp"
#include "NSTypes.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace NS
{
    class Data : public Copying<Data>
    {
    public:
        static Data* dataWithBytes(const void* pBytes, UInteger length);

        const void* bytes() const;

        void* mutableBytes() const;

        UInteger length() const;
    };
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

_NS_INLINE NS::Data* NS::Data::dataWithBytes(const void* pBytes, const UInteger length)
{
    return Object::sendMessage<Data*>(_NS_PRIVATE_CLS(NSData), _NS_PRIVATE_SEL(dataWithBytes_length_), pBytes, length);
}

_NS_INLINE const void* NS::Data::bytes() const
{
    return Object::sendMessage<const void*>(this, _NS_PRIVATE_SEL(bytes));
}

_NS_INLINE void* NS::Data::mutableBytes() const
{
    return Object::sendMessage<void*>(this, _NS_PRIVATE_SEL(mutableBytes));
}

_NS_INLINE NS::UInteger NS::Data::length() const
{
    return Object::sendMessage<UInteger>(this, _NS_PRIVATE_SEL(length));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
