#include "monokakido/core/platform/scoped_security_access.hpp"

#import <Foundation/Foundation.h>

#include <format>

namespace monokakido::platform::fs
{
    ScopedSecurityAccess::ScopedSecurityAccess(const std::filesystem::path& path)
    {
        @autoreleasepool {
            NSString* pathStr = [NSString stringWithUTF8String:path.c_str()];
            if (!pathStr) return;

            NSURL* url = [NSURL fileURLWithPath:pathStr];
            if (!url) return;

            if ([url startAccessingSecurityScopedResource])
            {
                // Manually retain since we're not using ARC
                url_ = (__bridge void*)[url retain];
                valid_ = true;
            }
        }
    }

    ScopedSecurityAccess::~ScopedSecurityAccess()
    {
        if (valid_ && url_) {
            @autoreleasepool {
                NSURL* url = (__bridge NSURL*)url_;
                [url stopAccessingSecurityScopedResource];
                [url release]; // Manually release
            }
        }

        url_ = nullptr;
        valid_ = false;
    }


    ScopedSecurityAccess::ScopedSecurityAccess(ScopedSecurityAccess&& other) noexcept
        : url_(other.url_), valid_(other.valid_)
    {
        other.url_ = nullptr;
        other.valid_ = false;
    }

    ScopedSecurityAccess& ScopedSecurityAccess::operator=(ScopedSecurityAccess&& other) noexcept
    {
        if (this != &other)
        {
            // Release current resource
            if (valid_ && url_)
            {
                @autoreleasepool
                {
                    NSURL* url = (__bridge NSURL*)url_;
                    [url stopAccessingSecurityScopedResource];
                    [url release]; // Manually release
                }
            }

            // Take ownership
            url_ = other.url_;
            valid_ = other.valid_;
            other.url_ = nullptr;
            other.valid_ = false;
        }
        return *this;
    }


    bool ScopedSecurityAccess::isValid() const
    {
        return valid_;
    }


    std::expected<BookmarkAccess, std::string> restoreAccessFromBookmark(const std::vector<uint8_t>& bookmarkData)
    {
        @autoreleasepool
        {
            NSData* data = [NSData dataWithBytes:bookmarkData.data()
                                          length:bookmarkData.size()];

            BOOL isStale = NO;
            NSError* error = nil;
            NSURL* url = [NSURL URLByResolvingBookmarkData:data
                options:NSURLBookmarkResolutionWithSecurityScope
                relativeToURL:nil
                bookmarkDataIsStale:&isStale
                error:&error];

            if (!url || error)
                return std::unexpected(std::format("Failed to resolve bookmark: {}", error.localizedDescription.UTF8String));

            if (isStale)
                return std::unexpected("Bookmark is stale, please re-grant access");

            auto path = std::filesystem::path{url.path.UTF8String};
            auto access = ScopedSecurityAccess(path);

            if (!access.isValid())
                return std::unexpected("Failed to access security-scoped resource");

            return BookmarkAccess{std::move(path), std::move(access)};
        }
    }
}