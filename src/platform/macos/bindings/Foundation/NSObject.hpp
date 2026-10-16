// Copyright (c) 2026 kiwakiwaa
// https://github.com/kiwakiwaa
// SPDX-License-Identifier: GPL-3.0-only
// Licensed under the GNU General Public License v3.0.

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
//
// Foundation/NSObject.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

#include "NSDefines.hpp"
#include "NSPrivate.hpp"
#include "NSTypes.hpp"

#include <objc/message.h>
#include <type_traits>

namespace NS
{
    template<class _Class, class _Base = class Object>
    class _NS_EXPORT Referencing : public _Base
    {
    public:
        _Class* autorelease();
    };

    class Object : public Referencing<Object, objc_object>
    {
    protected:
        template<class _Class>
        static _Class* alloc(const void* pClass);

        template<class _Class>
        _Class* init();

        template<typename _Ret, typename... _Args>
        static _Ret sendMessage(const void* pObj, SEL selector, _Args... args);

    private:
        Object() = delete;
        Object(const Object&) = delete;
        ~Object() = delete;
        Object& operator=(const Object&) = delete;
    };
}

template<class _Class, class _Base>
_NS_INLINE _Class* NS::Referencing<_Class, _Base>::autorelease()
{
    return Object::sendMessage<_Class*>(this, _NS_PRIVATE_SEL(autorelease));
}

template<typename _Ret, typename... _Args>
_NS_INLINE _Ret NS::Object::sendMessage(const void* pObj, SEL selector, _Args... args)
{
    // Floating-point and aggregate results require different dispatch on some architectures.
    static_assert(std::is_pointer_v<_Ret> || std::is_integral_v<_Ret> || std::is_void_v<_Ret>);
    using SendMessageProc = _Ret (*)(const void*, SEL, _Args...);
    return reinterpret_cast<SendMessageProc>(&objc_msgSend)(pObj, selector, args...);
}

template<class _Class>
_NS_INLINE _Class* NS::Object::alloc(const void* pClass)
{
    return sendMessage<_Class*>(pClass, _NS_PRIVATE_SEL(alloc));
}

template<class _Class>
_NS_INLINE _Class* NS::Object::init()
{
    return sendMessage<_Class*>(this, _NS_PRIVATE_SEL(init));
}
