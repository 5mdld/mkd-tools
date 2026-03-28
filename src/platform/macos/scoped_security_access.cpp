//
// kiwakiwaaにより 2026/01/15 に作成されました。
//

#include "scoped_security_access.hpp"

#include <Foundation/Foundation.hpp>

#include <format>

namespace MKD::macOS
{
    ScopedSecurityAccess::ScopedSecurityAccess(const fs::path& path)
    {
        NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

        const auto pathUtf8 = path.string();
        const NS::String* pathStr = NS::String::stringWithUTF8String(pathUtf8.c_str());
        if (!pathStr)
        {
            pool->drain();
            return;
        }

        NS::URL* url = NS::URL::fileURLWithPath(pathStr);
        if (!url)
        {
            pool->drain();
            return;
        }

        if (url->startAccessingSecurityScopedResource())
            url_ = static_cast<void*>(url->retain());

        pool->drain();
    }


    ScopedSecurityAccess::~ScopedSecurityAccess() { release(); }


    ScopedSecurityAccess::ScopedSecurityAccess(ScopedSecurityAccess&& other) noexcept
        : url_(other.url_)
    {
        other.url_ = nullptr;
    }


    ScopedSecurityAccess& ScopedSecurityAccess::operator=(ScopedSecurityAccess&& other) noexcept
    {
        if (this != &other)
        {
            release();
            url_ = other.url_;
            other.url_ = nullptr;
        }
        return *this;
    }


    bool ScopedSecurityAccess::isValid() const { return url_ != nullptr; }


    void ScopedSecurityAccess::release() noexcept
    {
        if (url_)
        {
            NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
            auto* url = static_cast<NS::URL*>(url_);
            url->stopAccessingSecurityScopedResource();
            url->release();
            pool->drain();
            url_ = nullptr;
        }
    }


    Result<BookmarkAccess> restoreAccessFromBookmark(const std::vector<uint8_t>& bookmarkData)
    {
        NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

        const NS::Data* data = NS::Data::dataWithBytes(bookmarkData.data(), bookmarkData.size());

        NS::Boolean isStale = 0;
        NS::Error* error = nullptr;
        const NS::URL* url = NS::URL::URLByResolvingBookmarkData(
            data,
            NS::URLBookmarkResolutionWithSecurityScope,
            nullptr,
            &isStale,
            &error
        );

        if (!url || error)
        {
            const NS::String* errorDescription = error ? error->localizedDescription() : nullptr;
            const char* errorText = (errorDescription && errorDescription->utf8String())
                                        ? errorDescription->utf8String()
                                        : "Unknown error";
            pool->drain();
            return std::unexpected(std::format("Failed to resolve bookmark: {}", errorText));
        }

        if (isStale != 0)
        {
            pool->drain();
            return std::unexpected("Bookmark is stale, please re-grant access");
        }

        const NS::String* pathString = url->path();
        auto path = fs::path{pathString ? pathString->utf8String() : ""};
        auto access = ScopedSecurityAccess(path);

        if (!access.isValid())
        {
            pool->drain();
            return std::unexpected("Failed to access security-scoped resource");
        }

        pool->drain();
        return BookmarkAccess{std::move(path), std::move(access)};
    }


    std::optional<BookmarkData> createSecurityScopedBookmark(const fs::path& path)
    {
        NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

        const auto pathUtf8 = path.string();
        const NS::String* pathStr = NS::String::stringWithUTF8String(pathUtf8.c_str());
        const NS::URL* url = NS::URL::fileURLWithPath(pathStr);
        if (!url)
        {
            pool->drain();
            return std::nullopt;
        }

        NS::Error* error = nullptr;
        constexpr auto bookmarkOptions = NS::URLBookmarkCreationWithSecurityScope |
                                         NS::URLBookmarkCreationSecurityScopeAllowOnlyReadAccess;
        const NS::Data* bookmark = url->bookmarkDataWithOptions(bookmarkOptions, nullptr, nullptr, &error);

        if (!bookmark || error)
        {
            pool->drain();
            return std::nullopt;
        }

        BookmarkData result;
        result.data.assign(
            static_cast<const uint8_t*>(bookmark->bytes()),
            static_cast<const uint8_t*>(bookmark->bytes()) + bookmark->length()
        );
        result.resolvedPath = path;

        pool->drain();
        return result;
    }
}
