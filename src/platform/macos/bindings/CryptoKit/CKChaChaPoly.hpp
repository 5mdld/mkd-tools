// Copyright (c) 2026 kiwakiwaa
// https://github.com/kiwakiwaa
// SPDX-License-Identifier: GPL-3.0-only
// Licensed under the GNU General Public License v3.0.

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
//
// CryptoKit/CKChaChaPoly.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "CKDefines.hpp"
#include "CKPrivate.hpp"
#include "CKSymmetricKey.hpp"
#include "CKSealedBox.hpp"
#include <Foundation/Foundation.hpp>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace CK
{
    class ChaChaPoly : public NS::Referencing<ChaChaPoly>
    {
    public:
        static SealedBox* seal(NS::Data* pData, SymmetricKey* pKey);

        static NS::Data* open(SealedBox* pSealedBox, SymmetricKey* pKey);
    };
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

_CK_INLINE CK::SealedBox* CK::ChaChaPoly::seal(NS::Data* pData, CK::SymmetricKey* pKey)
{
    return NS::Object::sendMessage<SealedBox*>(_CK_PRIVATE_CLS(CKChaChaPoly), _CK_PRIVATE_SEL(seal_using_), pData, pKey);
}

_CK_INLINE NS::Data* CK::ChaChaPoly::open(CK::SealedBox* pSealedBox, CK::SymmetricKey* pKey)
{
    return NS::Object::sendMessage<NS::Data*>(_CK_PRIVATE_CLS(CKChaChaPoly), _CK_PRIVATE_SEL(open_using_), pSealedBox, pKey);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
