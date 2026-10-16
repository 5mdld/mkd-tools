// Copyright (c) 2026 kiwakiwaa
// https://github.com/kiwakiwaa
// SPDX-License-Identifier: GPL-3.0-only
// Licensed under the GNU General Public License v3.0.

#pragma once

#include "NSData.hpp"
#include "NSError.hpp"
#include "NSString.hpp"

namespace NS
{
    enum URLBookmarkCreationOptions : UInteger
    {
        URLBookmarkCreationWithSecurityScope = (1ULL << 11),
        URLBookmarkCreationSecurityScopeAllowOnlyReadAccess = (1ULL << 12),
    };

    enum URLBookmarkResolutionOptions : UInteger
    {
        URLBookmarkResolutionWithSecurityScope = (1ULL << 10),
    };

    class URL : public Copying<URL>
    {
    public:
        static URL* fileURLWithPath(const String* pPath);

        static URL* URLByResolvingBookmarkData(const Data* pData, UInteger options, const URL* pRelativeToURL,
                                               Boolean* pIsStale, Error** ppError);

        String* path() const;

        Boolean startAccessingSecurityScopedResource() const;

        void stopAccessingSecurityScopedResource() const;

        Data* bookmarkDataWithOptions(UInteger options, const Object* pResourceValuesForKeys, const URL* pRelativeToURL,
                                      Error** ppError) const;
    };
}

_NS_INLINE NS::URL* NS::URL::fileURLWithPath(const String* pPath)
{
    return Object::sendMessage<URL*>(_NS_PRIVATE_CLS(NSURL), _NS_PRIVATE_SEL(fileURLWithPath_), pPath);
}

_NS_INLINE NS::URL* NS::URL::URLByResolvingBookmarkData(const Data* pData, const UInteger options,
                                                        const URL* pRelativeToURL, Boolean* pIsStale, Error** ppError)
{
    return Object::sendMessage<URL*>(
        _NS_PRIVATE_CLS(NSURL),
        _NS_PRIVATE_SEL(URLByResolvingBookmarkData_options_relativeToURL_bookmarkDataIsStale_error_),
        pData,
        options,
        pRelativeToURL,
        pIsStale,
        ppError
    );
}

_NS_INLINE NS::String* NS::URL::path() const
{
    return Object::sendMessage<String*>(this, _NS_PRIVATE_SEL(path));
}

_NS_INLINE NS::Boolean NS::URL::startAccessingSecurityScopedResource() const
{
    return Object::sendMessage<Boolean>(this, _NS_PRIVATE_SEL(startAccessingSecurityScopedResource));
}

_NS_INLINE void NS::URL::stopAccessingSecurityScopedResource() const
{
    Object::sendMessage<void>(this, _NS_PRIVATE_SEL(stopAccessingSecurityScopedResource));
}

_NS_INLINE NS::Data* NS::URL::bookmarkDataWithOptions(const UInteger options, const Object* pResourceValuesForKeys,
                                                      const URL* pRelativeToURL, Error** ppError) const
{
    return Object::sendMessage<Data*>(
        this,
        _NS_PRIVATE_SEL(bookmarkDataWithOptions_includingResourceValuesForKeys_relativeToURL_error_),
        options,
        pResourceValuesForKeys,
        pRelativeToURL,
        ppError
    );
}
