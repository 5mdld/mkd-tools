// Copyright (c) 2026 kiwakiwaa
// https://github.com/kiwakiwaa
// SPDX-License-Identifier: GPL-3.0-only
// Licensed under the GNU General Public License v3.0.

#pragma once

#include "NSString.hpp"

namespace NS
{
    class URL : public Referencing<URL>
    {
    public:
        String* path() const;
    };
}

_NS_INLINE NS::String* NS::URL::path() const
{
    return Object::sendMessage<String*>(this, _NS_PRIVATE_SEL(path));
}
