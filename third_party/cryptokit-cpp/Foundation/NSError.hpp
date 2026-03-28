// Copyright (c) 2026 kiwakiwaa
// https://github.com/kiwakiwaa
// SPDX-License-Identifier: GPL-3.0-only
// Licensed under the GNU General Public License v3.0.

#pragma once

#include "NSObject.hpp"

namespace NS
{
    class String;

    class Error : public Referencing<Error>
    {
    public:
        String* localizedDescription() const;
    };
}

_NS_INLINE NS::String* NS::Error::localizedDescription() const
{
    return Object::sendMessage<String*>(this, _NS_PRIVATE_SEL(localizedDescription));
}
