// Copyright (c) 2026 kiwakiwaa
// https://github.com/kiwakiwaa
// SPDX-License-Identifier: GPL-3.0-only
// Licensed under the GNU General Public License v3.0.

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
//
// CryptoKit/CKSealedBox.hpp
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
    class SealedBox : public NS::Referencing<SealedBox>
    {
    public:
        static SealedBox* alloc();

        static SealedBox* sealedBox(NS::Data* pCombined);

        SealedBox* init(NS::Data* pCombined);

        NS::Data* combined() const;

        NS::Data* nonce() const;

        NS::Data* ciphertext() const;

        NS::Data* tag() const;
    };
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

_CK_INLINE CK::SealedBox* CK::SealedBox::alloc()
{
    return NS::Object::sendMessage<SealedBox*>(_CK_PRIVATE_CLS(CKSealedBox), _NS_PRIVATE_SEL(alloc));
}

_CK_INLINE CK::SealedBox* CK::SealedBox::sealedBox(NS::Data* pCombined)
{
    auto pSealedBox = alloc()->init(pCombined);
    return pSealedBox ? pSealedBox->autorelease() : nullptr;
}

_CK_INLINE CK::SealedBox* CK::SealedBox::init(NS::Data* pCombined)
{
    return NS::Object::sendMessage<SealedBox*>(this, _CK_PRIVATE_SEL(initWithCombined_), pCombined);
}

_CK_INLINE NS::Data* CK::SealedBox::combined() const
{
    return NS::Object::sendMessage<NS::Data*>(this, _CK_PRIVATE_SEL(combined));
}

_CK_INLINE NS::Data* CK::SealedBox::nonce() const
{
    return NS::Object::sendMessage<NS::Data*>(this, _CK_PRIVATE_SEL(nonce));
}

_CK_INLINE NS::Data* CK::SealedBox::ciphertext() const
{
    return NS::Object::sendMessage<NS::Data*>(this, _CK_PRIVATE_SEL(ciphertext));
}

_CK_INLINE NS::Data* CK::SealedBox::tag() const
{
    return NS::Object::sendMessage<NS::Data*>(this, _CK_PRIVATE_SEL(tag));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
