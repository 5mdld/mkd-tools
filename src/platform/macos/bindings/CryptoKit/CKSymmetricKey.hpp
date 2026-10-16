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

#include "CKDefines.hpp"
#include "CKPrivate.hpp"
#include <Foundation/Foundation.hpp>

namespace CK
{
    class SymmetricKey : public NS::Referencing<SymmetricKey>
    {
    public:
        static SymmetricKey* alloc();

        static SymmetricKey* symmetricKey(NS::Data* pData);

        SymmetricKey* init(NS::Data* pData);
    };
}

_CK_INLINE CK::SymmetricKey* CK::SymmetricKey::alloc()
{
    return NS::Object::sendMessage<SymmetricKey*>(_CK_PRIVATE_CLS(CKSymmetricKey), _NS_PRIVATE_SEL(alloc));
}

_CK_INLINE CK::SymmetricKey* CK::SymmetricKey::symmetricKey(NS::Data* pData)
{
    auto pKey = alloc()->init(pData);
    return pKey ? pKey->autorelease() : nullptr;
}

_CK_INLINE CK::SymmetricKey* CK::SymmetricKey::init(NS::Data* pData)
{
    return NS::Object::sendMessage<SymmetricKey*>(this, _CK_PRIVATE_SEL(initWithData_), pData);
}
