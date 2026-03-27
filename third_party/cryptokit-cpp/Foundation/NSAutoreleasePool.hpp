// Copyright (c) 2026 kiwakiwaa
// https://github.com/kiwakiwaa
// SPDX-License-Identifier: GPL-3.0-only
// Licensed under the GNU General Public License v3.0.

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
//
// Foundation/NSAutoreleasePool.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "NSObject.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace NS
{
    class AutoreleasePool : public Referencing<AutoreleasePool>
    {
    public:
        static AutoreleasePool* alloc();

        AutoreleasePool* init();

        void drain();
    };
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

_NS_INLINE NS::AutoreleasePool* NS::AutoreleasePool::alloc()
{
    return Object::alloc<AutoreleasePool>(_NS_PRIVATE_CLS(NSAutoreleasePool));
}

_NS_INLINE NS::AutoreleasePool* NS::AutoreleasePool::init()
{
    return Object::init<AutoreleasePool>();
}

_NS_INLINE void NS::AutoreleasePool::drain()
{
    Object::sendMessage<void>(this, _NS_PRIVATE_SEL(drain));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
