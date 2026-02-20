#include "MKD/platform/macos/fs.hpp"

#import <Foundation/Foundation.h>

namespace MKD::macOS
{
    std::filesystem::path getContainerPathByGroupIdentifier(const std::string& groupIdentifier)
    {
        @autoreleasepool
        {
              NSString* groupId = [NSString stringWithUTF8String:groupIdentifier.c_str()];
              NSURL* containerURL = [[NSFileManager defaultManager] containerURLForSecurityApplicationGroupIdentifier:groupId];
              if (!containerURL)
                  return {};

              return std::filesystem::path{containerURL.path.UTF8String};
        }
    }
}