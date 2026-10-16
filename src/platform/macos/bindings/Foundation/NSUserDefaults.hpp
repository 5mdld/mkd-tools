// Copyright (c) 2026 kiwakiwaa
// https://github.com/kiwakiwaa
// SPDX-License-Identifier: GPL-3.0-only
// Licensed under the GNU General Public License v3.0.

#pragma once

#include "NSData.hpp"
#include "NSString.hpp"

namespace NS
{
    class UserDefaults : public Referencing<UserDefaults>
    {
    public:
        static UserDefaults* standardUserDefaults();

        Data* dataForKey(const String* pKey) const;

        void setObject(const Object* pObject, const String* pKey) const;

        void removeObjectForKey(const String* pKey) const;
    };
}

_NS_INLINE NS::UserDefaults* NS::UserDefaults::standardUserDefaults()
{
    return Object::sendMessage<UserDefaults*>(_NS_PRIVATE_CLS(NSUserDefaults), _NS_PRIVATE_SEL(standardUserDefaults));
}

_NS_INLINE NS::Data* NS::UserDefaults::dataForKey(const String* pKey) const
{
    return Object::sendMessage<Data*>(this, _NS_PRIVATE_SEL(dataForKey_), pKey);
}

_NS_INLINE void NS::UserDefaults::setObject(const Object* pObject, const String* pKey) const
{
    Object::sendMessage<void>(this, _NS_PRIVATE_SEL(setObject_forKey_), pObject, pKey);
}

_NS_INLINE void NS::UserDefaults::removeObjectForKey(const String* pKey) const
{
    Object::sendMessage<void>(this, _NS_PRIVATE_SEL(removeObjectForKey_), pKey);
}
