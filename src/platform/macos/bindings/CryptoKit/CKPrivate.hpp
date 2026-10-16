//
// kiwakiwaaにより 2026/03/27 に作成されました。
//

// Adapted from Apple's metal-cpp. Original portions:
// Copyright 2020-2024 Apple Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Modifications copyright (c) 2026 kiwakiwaa.
// Modifications are licensed under the GNU General Public License v3.0 only.
// SPDX-License-Identifier: Apache-2.0 AND GPL-3.0-only
// See LICENSE-APACHE-2.0.txt in the bindings directory.

#pragma once

#include "CKDefines.hpp"
#include <objc/runtime.h>

#define _CK_PRIVATE_CLS(symbol) (CK::Private::Class::s_k##symbol)
#define _CK_PRIVATE_SEL(accessor) (CK::Private::Selector::s_k##accessor)

#if defined(CK_PRIVATE_IMPLEMENTATION)
#define _CK_PRIVATE_VISIBILITY __attribute__((visibility("default")))
#define _CK_PRIVATE_DEF_CLS(symbol) void* s_k##symbol _CK_PRIVATE_VISIBILITY = CK::Private::lookUpClass(#symbol)
#define _CK_PRIVATE_DEF_SEL(accessor, symbol) SEL s_k##accessor _CK_PRIVATE_VISIBILITY = sel_registerName(symbol)
#else
#define _CK_PRIVATE_DEF_CLS(symbol) extern void* s_k##symbol
#define _CK_PRIVATE_DEF_SEL(accessor, symbol) extern SEL s_k##accessor
#endif

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
            _CK_PRIVATE_DEF_SEL(initWithData_, "initWithData:");

            // CKSealedBox
            _CK_PRIVATE_DEF_SEL(initWithCombined_, "initWithCombined:");
            _CK_PRIVATE_DEF_SEL(combined, "combined");

            // CKChaChaPoly
            _CK_PRIVATE_DEF_SEL(seal_using_, "seal:using:");
            _CK_PRIVATE_DEF_SEL(open_using_, "open:using:");
        }
    }
}
