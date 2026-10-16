// Copyright (c) 2026 kiwakiwaa
// https://github.com/kiwakiwaa
// SPDX-License-Identifier: GPL-3.0-only
// Licensed under the GNU General Public License v3.0.

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
//
// CryptoKit/CKSymmetricKey.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "CKDefines.hpp"
#include "CKPrivate.hpp"
#include <Foundation/Foundation.hpp>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace CK
{
    class SymmetricKey : public NS::Referencing<SymmetricKey>
    {
    public:
        static SymmetricKey* alloc();

        static SymmetricKey* symmetricKey(NS::UInteger bitCount);

        static SymmetricKey* symmetricKey(NS::Data* pData);

        SymmetricKey* init(NS::UInteger bitCount);

        SymmetricKey* init(NS::Data* pData);

        NS::Data* rawData() const;
    };
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

_CK_INLINE CK::SymmetricKey* CK::SymmetricKey::alloc()
{
    return NS::Object::sendMessage<SymmetricKey*>(_CK_PRIVATE_CLS(CKSymmetricKey), _NS_PRIVATE_SEL(alloc));
}

_CK_INLINE CK::SymmetricKey* CK::SymmetricKey::symmetricKey(const NS::UInteger bitCount)
{
    auto pKey = alloc()->init(bitCount);
    return pKey ? pKey->autorelease() : nullptr;
}

_CK_INLINE CK::SymmetricKey* CK::SymmetricKey::symmetricKey(NS::Data* pData)
{
    auto pKey = alloc()->init(pData);
    return pKey ? pKey->autorelease() : nullptr;
}

_CK_INLINE CK::SymmetricKey* CK::SymmetricKey::init(const NS::UInteger bitCount)
{
    return NS::Object::sendMessage<SymmetricKey*>(this, _CK_PRIVATE_SEL(initWithBitCount_), bitCount);
}

_CK_INLINE CK::SymmetricKey* CK::SymmetricKey::init(NS::Data* pData)
{
    return NS::Object::sendMessage<SymmetricKey*>(this, _CK_PRIVATE_SEL(initWithData_), pData);
}

_CK_INLINE NS::Data* CK::SymmetricKey::rawData() const
{
    return NS::Object::sendMessage<NS::Data*>(this, _CK_PRIVATE_SEL(rawData));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
