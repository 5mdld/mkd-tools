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
