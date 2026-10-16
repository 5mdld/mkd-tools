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

#define _CK_PRIVATE_CLS(symbol) (CK::Private::Class::s_k##symbol)
#define _CK_PRIVATE_SEL(accessor) (CK::Private::Selector::s_k##accessor)

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#if defined(CK_PRIVATE_IMPLEMENTATION)
#define _CK_PRIVATE_VISIBILITY __attribute__((visibility("default")))
#define _CK_PRIVATE_DEF_CLS(symbol) void* s_k##symbol _CK_PRIVATE_VISIBILITY = CK::Private::lookUpClass(#symbol)
#define _CK_PRIVATE_DEF_SEL(accessor, symbol) SEL s_k##accessor _CK_PRIVATE_VISIBILITY = sel_registerName(symbol)
#else
#define _CK_PRIVATE_DEF_CLS(symbol) extern void* s_k##symbol
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

        _CK_INLINE void* lookUpClass(const char* pClassName)
        {
            Initialize();
#ifdef __OBJC__
            return (__bridge void*)objc_lookUpClass(pClassName);
#else
            return objc_lookUpClass(pClassName);
#endif
        }

        namespace Class
        {
            _CK_PRIVATE_DEF_CLS(CKSymmetricKey);
            _CK_PRIVATE_DEF_CLS(CKSealedBox);
            _CK_PRIVATE_DEF_CLS(CKChaChaPoly);
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
