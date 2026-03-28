//
// kiwakiwaaにより 2026/02/19 に作成されました。
//

#include "fs.hpp"

#include <Foundation/Foundation.hpp>

namespace MKD::macOS
{
    std::filesystem::path getContainerPathByGroupIdentifier(const std::string& groupIdentifier)
    {
        NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

        const NS::String* groupId = NS::String::stringWithUTF8String(groupIdentifier.c_str());
        const NS::URL* containerURL = NS::FileManager::defaultManager()->
                containerURLForSecurityApplicationGroupIdentifier(groupId);
        if (!containerURL)
        {
            pool->drain();
            return {};
        }

        const NS::String* pathString = containerURL->path();
        std::filesystem::path result{pathString ? pathString->utf8String() : ""};

        pool->drain();
        return result;
    }

    std::optional<std::filesystem::path> getMonokakidoDictionariesPath(const std::string& groupIdentifier,
                                                                       const std::string& relativePath)
    {
        const auto container = getContainerPathByGroupIdentifier(groupIdentifier);
        if (container.empty()) return std::nullopt;

        auto path = container / relativePath;
        if (!std::filesystem::exists(path)) return std::nullopt;

        return path;
    }
}
