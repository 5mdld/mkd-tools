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

#include <objc/runtime.h>

#define _NS_PRIVATE_CLS(symbol) (NS::Private::Class::s_k##symbol)
#define _NS_PRIVATE_SEL(accessor) (NS::Private::Selector::s_k##accessor)

#if defined(NS_PRIVATE_IMPLEMENTATION)

#ifdef __OBJC__
#define _NS_PRIVATE_OBJC_LOOKUP_CLASS(symbol) ((__bridge void*)objc_lookUpClass(#symbol))
#else
#define _NS_PRIVATE_OBJC_LOOKUP_CLASS(symbol) objc_lookUpClass(#symbol)
#endif // __OBJC__

#define _NS_PRIVATE_VISIBILITY __attribute__((visibility("default")))
#define _NS_PRIVATE_DEF_CLS(symbol) void* s_k##symbol _NS_PRIVATE_VISIBILITY = _NS_PRIVATE_OBJC_LOOKUP_CLASS(symbol)
#define _NS_PRIVATE_DEF_SEL(accessor, symbol) SEL s_k##accessor _NS_PRIVATE_VISIBILITY = sel_registerName(symbol)

#else

#define _NS_PRIVATE_DEF_CLS(symbol) extern void* s_k##symbol
#define _NS_PRIVATE_DEF_SEL(accessor, symbol) extern SEL s_k##accessor

#endif // NS_PRIVATE_IMPLEMENTATION

namespace NS::Private::Class
{
    _NS_PRIVATE_DEF_CLS(NSAutoreleasePool);
    _NS_PRIVATE_DEF_CLS(NSData);
    _NS_PRIVATE_DEF_CLS(NSFileManager);
    _NS_PRIVATE_DEF_CLS(NSOrthography);
    _NS_PRIVATE_DEF_CLS(NSString);
}

namespace NS::Private::Selector
{
    _NS_PRIVATE_DEF_SEL(alloc, "alloc");
    _NS_PRIVATE_DEF_SEL(autorelease, "autorelease");
    _NS_PRIVATE_DEF_SEL(bytes, "bytes");
    _NS_PRIVATE_DEF_SEL(containerURLForSecurityApplicationGroupIdentifier_, "containerURLForSecurityApplicationGroupIdentifier:");
    _NS_PRIVATE_DEF_SEL(count, "count");
    _NS_PRIVATE_DEF_SEL(dataWithBytes_length_, "dataWithBytes:length:");
    _NS_PRIVATE_DEF_SEL(defaultManager, "defaultManager");
    _NS_PRIVATE_DEF_SEL(defaultOrthographyForLanguage_, "defaultOrthographyForLanguage:");
    _NS_PRIVATE_DEF_SEL(drain, "drain");
    _NS_PRIVATE_DEF_SEL(init, "init");
    _NS_PRIVATE_DEF_SEL(initWithBytes_length_encoding_, "initWithBytes:length:encoding:");
    _NS_PRIVATE_DEF_SEL(length, "length");
    _NS_PRIVATE_DEF_SEL(linguisticTagsInRange_scheme_options_orthography_tokenRanges_, "linguisticTagsInRange:scheme:options:orthography:tokenRanges:");
    _NS_PRIVATE_DEF_SEL(objectAtIndex_, "objectAtIndex:");
    _NS_PRIVATE_DEF_SEL(path, "path");
    _NS_PRIVATE_DEF_SEL(stringWithUTF8String_, "stringWithUTF8String:");
    _NS_PRIVATE_DEF_SEL(UTF8String, "UTF8String");
}
