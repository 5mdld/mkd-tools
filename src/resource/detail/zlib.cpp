//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#include "zlib.hpp"

#include <zlib.h>
#include <algorithm>
#include <format>
#include <limits>
#include <memory>

namespace MKD::detail::zlib
{
    Result<std::vector<uint8_t>> compress(std::span<const uint8_t> data, const int level)
    {
        if (level < Z_DEFAULT_COMPRESSION || level > Z_BEST_COMPRESSION)
            return std::unexpected("Invalid compression level, expected -1 through 9");
        if (data.size() > std::numeric_limits<uInt>::max())
            return std::unexpected("Compression input exceeds the supported size");

        uLongf size = compressBound(data.size());
        std::vector<uint8_t> output(size);
        const int result = compress2(output.data(), &size, data.data(), data.size(), level);
        if (result != Z_OK)
            return std::unexpected(std::format("Zlib compression failed: {}", result));

        output.resize(size);
        return output;
    }

    Result<std::vector<uint8_t>> decompress(std::span<const uint8_t> data, const size_t initialSize)
    {
        if (data.size() > std::numeric_limits<uInt>::max())
            return std::unexpected("Decompression input exceeds the supported size");

        std::vector<uint8_t> output(std::clamp(initialSize, size_t{1}, size_t{1024 * 1024}));
        z_stream stream{};
        stream.next_in = const_cast<Bytef*>(data.data());
        stream.avail_in = static_cast<uInt>(data.size());
        if (inflateInit(&stream) != Z_OK)
            return std::unexpected("Failed to initialise zlib decompression");

        const std::unique_ptr<z_stream, decltype(&inflateEnd)> cleanup(&stream, inflateEnd);
        while (true)
        {
            const size_t produced = stream.total_out;
            if (produced == output.size())
            {
                if (output.size() > std::numeric_limits<uInt>::max() / 2)
                    return std::unexpected("Decompressed data exceeds the supported size");
                output.resize(output.size() * 2);
            }
            stream.next_out = output.data() + produced;
            stream.avail_out = static_cast<uInt>(output.size() - produced);
            const int result = inflate(&stream, Z_NO_FLUSH);
            if (result == Z_STREAM_END)
            {
                output.resize(stream.total_out);
                return output;
            }
            if (result != Z_OK || (stream.avail_in == 0 && stream.avail_out != 0))
                return std::unexpected(std::format("Incomplete or invalid zlib stream: {}", result));
        }
    }

    bool isCompressed(std::span<const uint8_t> data) noexcept
    {
        return data.size() >= 2 && (data[0] & 0x0F) == Z_DEFLATED &&
            (data[0] >> 4) <= 7 &&
            ((static_cast<unsigned>(data[0]) << 8) | data[1]) % 31 == 0;
    }
}
