// Copyright (c) 2026 kiwakiwaa
// https://github.com/kiwakiwaa
// SPDX-License-Identifier: GPL-3.0-only
// Licensed under the GNU General Public License v3.0.

#pragma once

#include "NSURL.hpp"

namespace NS
{
    class FileManager : public Referencing<FileManager>
    {
    public:
        static FileManager* defaultManager();

        URL* containerURLForSecurityApplicationGroupIdentifier(const String* pGroupIdentifier) const;
    };
}

_NS_INLINE NS::FileManager* NS::FileManager::defaultManager()
{
    return Object::sendMessage<FileManager*>(_NS_PRIVATE_CLS(NSFileManager), _NS_PRIVATE_SEL(defaultManager));
}

_NS_INLINE NS::URL* NS::FileManager::containerURLForSecurityApplicationGroupIdentifier(const String* pGroupIdentifier) const
{
    return Object::sendMessage<URL*>(this, _NS_PRIVATE_SEL(containerURLForSecurityApplicationGroupIdentifier_), pGroupIdentifier);
}
