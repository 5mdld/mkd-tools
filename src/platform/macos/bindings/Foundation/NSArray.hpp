//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

// Copyright (c) 2026 kiwakiwaa
// https://github.com/kiwakiwaa
// SPDX-License-Identifier: GPL-3.0-only
// Licensed under the GNU General Public License v3.0.

#pragma once

#include "NSObject.hpp"

namespace NS
{
    class Array : public Referencing<Array>
    {
    public:
        UInteger count() const;

        Object* objectAtIndex(UInteger index) const;
    };
}

_NS_INLINE NS::UInteger NS::Array::count() const
{
    return Object::sendMessage<UInteger>(this, _NS_PRIVATE_SEL(count));
}

_NS_INLINE NS::Object* NS::Array::objectAtIndex(const UInteger index) const
{
    return Object::sendMessage<Object*>(this, _NS_PRIVATE_SEL(objectAtIndex_), index);
}
