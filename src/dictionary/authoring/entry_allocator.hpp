//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#pragma once
#include "MKD/dictionary/dictionary_writer.hpp"

namespace MKD
{
    enum class CreationProfile;
}

#include "dictionary/package/package_format.hpp"
#include <map>
#include <unordered_set>

namespace MKD::detail
{
    class EntryAllocator
    {
    public:
        explicit EntryAllocator(const CreationProfile profile) : profile_(profile)
        {
        }

        Result<uint32_t> reserve(std::optional<uint32_t> requested);
        Result<EntryId> addTarget(uint32_t page, EntryTargetKind kind);
        Result<void> recordPage(uint32_t page);
        void written(const uint32_t page) { reserved_.erase(page); }
        [[nodiscard]] bool reserved(uint32_t page) const { return reserved_.contains(page); }
        [[nodiscard]] const auto& pages() const { return pages_; }
        Result<void> setOrder(std::span<const uint32_t> pages);
        Result<void> validateReservations() const;
        Result<void> validateOrder() const;
        Result<void> writeOrder(const std::filesystem::path& root);

    private:
        CreationProfile profile_;
        std::unordered_set<uint32_t> pages_;
        std::map<uint32_t, std::array<uint32_t, native::entry::targetRanges.size()>> reserved_;
        uint32_t next_ = 1;
        std::vector<uint32_t> order_;
    };
}
