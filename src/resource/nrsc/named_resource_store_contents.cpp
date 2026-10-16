//
// kiwakiwaaにより 2026/02/28 に作成されました。
//

#include "named_resource_store_contents.hpp"
#include "resource/detail/zlib.hpp"

namespace MKD
{
    Result<NamedResourceStoreContents> NamedResourceStoreContents::open(const std::filesystem::path& directory)
    {
        auto files = detail::ResourceFiles::open(directory, ".nrsc");
        if (!files)
            return std::unexpected(files.error());
        return NamedResourceStoreContents(std::move(*files));
    }

    Result<RetainedSpan> NamedResourceStoreContents::get(const NamedResourceStoreIndexRecord& record) const
    {
        auto bytes = files_.read(record.fileSequence, record.fileOffset, record.length);
        if (!bytes)
            return std::unexpected(bytes.error());

        if (!record.isCompressed())
            return bytes;

        auto decoded = detail::zlib::decompress(bytes->span(), record.length);
        if (!decoded)
            return std::unexpected(decoded.error());

        return RetainedSpan(std::move(*decoded));
    }
}
