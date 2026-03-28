//
// kiwakiwaaにより 2026/02/27 に作成されました。
//

#include "bookmark_store.hpp"

#include <Foundation/Foundation.hpp>

namespace MKD::macOS
{
    namespace
    {
        NS::String* toNSString(const std::string_view sv)
        {
            auto* str = NS::String::alloc()->initWithBytes(sv.data(), sv.size(), NS::UTF8StringEncoding);
            return str ? str->autorelease() : nullptr;
        }
    }

    std::optional<std::vector<uint8_t>> loadSavedBookmark(std::string_view key)
    {
        NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

        const NS::Data* data = NS::UserDefaults::standardUserDefaults()->dataForKey(toNSString(key));
        if (!data)
        {
            pool->drain();
            return std::nullopt;
        }

        std::vector<uint8_t> result{
            static_cast<const uint8_t*>(data->bytes()),
            static_cast<const uint8_t*>(data->bytes()) + data->length()
        };

        pool->drain();
        return result;
    }

    void saveBookmark(const std::vector<uint8_t>& bookmarkData, std::string_view key)
    {
        NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

        const NS::Data* data = NS::Data::dataWithBytes(bookmarkData.data(), bookmarkData.size());
        NS::UserDefaults::standardUserDefaults()->setObject(data, toNSString(key));

        pool->drain();
    }

    void clearSavedBookmark(std::string_view key)
    {
        NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

        NS::UserDefaults::standardUserDefaults()->removeObjectForKey(toNSString(key));

        pool->drain();
    }
}
