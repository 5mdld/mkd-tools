// Copyright (c) 2026 kiwakiwaa
// https://github.com/kiwakiwaa
// SPDX-License-Identifier: GPL-3.0-only
// Licensed under the GNU General Public License v3.0.

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
//
// Foundation/NSPrivate.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <objc/runtime.h>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#define _NS_PRIVATE_CLS(symbol) (NS::Private::Class::s_k##symbol)
#define _NS_PRIVATE_SEL(accessor) (NS::Private::Selector::s_k##accessor)

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace NS::Private::Class
{
    _NS_PRIVATE_DEF_CLS(NSAutoreleasePool);
    _NS_PRIVATE_DEF_CLS(NSData);
    _NS_PRIVATE_DEF_CLS(NSObject);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace NS::Private::Selector
{
    _NS_PRIVATE_DEF_SEL(alloc, "alloc");
    _NS_PRIVATE_DEF_SEL(autorelease, "autorelease");
    _NS_PRIVATE_DEF_SEL(bytes, "bytes");
    _NS_PRIVATE_DEF_SEL(copy, "copy");
    _NS_PRIVATE_DEF_SEL(dataWithBytes_length_, "dataWithBytes:length:");
    _NS_PRIVATE_DEF_SEL(debugDescription, "debugDescription");
    _NS_PRIVATE_DEF_SEL(description, "description");
    _NS_PRIVATE_DEF_SEL(drain, "drain");
    _NS_PRIVATE_DEF_SEL(hash, "hash");
    _NS_PRIVATE_DEF_SEL(init, "init");
    _NS_PRIVATE_DEF_SEL(isEqual_, "isEqual:");
    _NS_PRIVATE_DEF_SEL(length, "length");
    _NS_PRIVATE_DEF_SEL(methodSignatureForSelector_, "methodSignatureForSelector:");
    _NS_PRIVATE_DEF_SEL(mutableBytes, "mutableBytes");
    _NS_PRIVATE_DEF_SEL(release, "release");
    _NS_PRIVATE_DEF_SEL(respondsToSelector_, "respondsToSelector:");
    _NS_PRIVATE_DEF_SEL(retain, "retain");
    _NS_PRIVATE_DEF_SEL(retainCount, "retainCount");
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
