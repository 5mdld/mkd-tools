//
// kiwakiwaaにより 2026/03/28 に作成されました。
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

#include "NSArray.hpp"
#include "NSLinguisticTagger.hpp"
#include "NSObject.hpp"
#include "NSOrthography.hpp"
#include "NSTypes.hpp"

namespace NS
{
    using StringEncoding = UInteger;

    enum : StringEncoding
    {
        UTF8StringEncoding = 4
    };

    class String : public Referencing<String>
    {
    public:
        static String* alloc();

        String* initWithBytes(const void* pBytes, UInteger length, StringEncoding encoding);

        static String* stringWithUTF8String(const char* pString);

        const char* utf8String() const;

        UInteger length() const;

        Array* linguisticTagsInRange(Range range, LinguisticTagScheme scheme, LinguisticTaggerOptions options,
                                     const Orthography* pOrthography, Array** pTokenRanges) const;
    };
}

_NS_INLINE NS::String* NS::String::alloc()
{
    return Object::alloc<String>(_NS_PRIVATE_CLS(NSString));
}

_NS_INLINE NS::String* NS::String::initWithBytes(const void* pBytes, const UInteger length, const StringEncoding encoding)
{
    return Object::sendMessage<String*>(this, _NS_PRIVATE_SEL(initWithBytes_length_encoding_), pBytes, length, encoding);
}

_NS_INLINE NS::String* NS::String::stringWithUTF8String(const char* pString)
{
    return Object::sendMessage<String*>(_NS_PRIVATE_CLS(NSString), _NS_PRIVATE_SEL(stringWithUTF8String_), pString);
}

_NS_INLINE const char* NS::String::utf8String() const
{
    return Object::sendMessage<const char*>(this, _NS_PRIVATE_SEL(UTF8String));
}

_NS_INLINE NS::UInteger NS::String::length() const
{
    return Object::sendMessage<UInteger>(this, _NS_PRIVATE_SEL(length));
}

_NS_INLINE NS::Array* NS::String::linguisticTagsInRange(const Range range, const LinguisticTagScheme scheme, const LinguisticTaggerOptions options,
                                                        const Orthography* pOrthography, Array** pTokenRanges) const
{
    return Object::sendMessage<Array*>(this, _NS_PRIVATE_SEL(linguisticTagsInRange_scheme_options_orthography_tokenRanges_), range, scheme, options, pOrthography, pTokenRanges);
}
