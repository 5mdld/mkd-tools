// Copyright (c) 2026 kiwakiwaa
// https://github.com/kiwakiwaa
// SPDX-License-Identifier: GPL-3.0-only
// Licensed under the GNU General Public License v3.0.

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
//
// CryptoKit/CKPrivate.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "CKDefines.hpp"
#include <objc/runtime.h>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#define _CK_PRIVATE_CLS(symbol) (CK::Private::Class::symbol())
#define _CK_PRIVATE_SEL(accessor) (CK::Private::Selector::s_k##accessor)

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#if defined(CK_PRIVATE_IMPLEMENTATION)
#define _CK_PRIVATE_VISIBILITY __attribute__((visibility("default")))
#define _CK_PRIVATE_DEF_SEL(accessor, symbol) SEL s_k##accessor _CK_PRIVATE_VISIBILITY = sel_registerName(symbol)
#else
#define _CK_PRIVATE_DEF_SEL(accessor, symbol) extern SEL s_k##accessor
#endif

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

extern "C" void CKBridgeInitialize();

namespace CK
{
    namespace Private
    {
        _CK_INLINE void Initialize()
        {
            static bool s_initialized = []() {
                CKBridgeInitialize();
                return true;
            }();
            (void) s_initialized;
        }

        namespace Class
        {
            _CK_INLINE void* CKSymmetricKey()
            {
                Initialize();
                return objc_lookUpClass("CKSymmetricKey");
            }

            _CK_INLINE void* CKSealedBox()
            {
                Initialize();
                return objc_lookUpClass("CKSealedBox");
            }

            _CK_INLINE void* CKChaChaPoly()
            {
                Initialize();
                return objc_lookUpClass("CKChaChaPoly");
            }
        }

        namespace Selector
        {
            // CKSymmetricKey
            _CK_PRIVATE_DEF_SEL(initWithBitCount_, "initWithBitCount:");
            _CK_PRIVATE_DEF_SEL(initWithData_, "initWithData:");
            _CK_PRIVATE_DEF_SEL(rawData, "rawData");

            // CKSealedBox
            _CK_PRIVATE_DEF_SEL(initWithCombined_, "initWithCombined:");
            _CK_PRIVATE_DEF_SEL(combined, "combined");
            _CK_PRIVATE_DEF_SEL(nonce, "nonce");
            _CK_PRIVATE_DEF_SEL(ciphertext, "ciphertext");
            _CK_PRIVATE_DEF_SEL(tag, "tag");

            // CKChaChaPoly
            _CK_PRIVATE_DEF_SEL(seal_using_, "seal:using:");
            _CK_PRIVATE_DEF_SEL(open_using_, "open:using:");
        }
    }
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
