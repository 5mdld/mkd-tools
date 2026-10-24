//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "dictionary/authoring/product_icon.hpp"
#include "dictionary/authoring/product_build.hpp"
#include "dictionary/package/package_format.hpp"

#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_MAX_DIMENSIONS 4096
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_IMAGE_RESIZE_STATIC
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

#include <limits>

namespace MKD::detail
{
    class IconImage
    {
    public:
        static Result<IconImage> read(std::span<const uint8_t> bytes)
        {
            if (bytes.size() > std::numeric_limits<int>::max())
                return std::unexpected("Icon image is too large");

            int width = 0, height = 0, channels = 0;
            const auto length = static_cast<int>(bytes.size());
            if (!stbi_info_from_memory(bytes.data(), length, &width, &height, &channels) ||
                width < 1 || width != height || width > STBI_MAX_DIMENSIONS)
                return std::unexpected("Icon must be a square PNG or JPEG, at most 4096 pixels wide");

            Pixels pixels(stbi_load_from_memory(bytes.data(), length, &width, &height, &channels, 4), stbi_image_free);
            if (!pixels)
                return std::unexpected("Cannot decode icon image");

            return IconImage(std::move(pixels), width);
        }

        [[nodiscard]] Result<std::vector<uint8_t>> png(const int size) const
        {
            std::vector<uint8_t> resized(static_cast<size_t>(size) * size * 4);
            if (!stbir_resize_uint8_srgb(pixels_.get(), size_, size_, 0,
                                         resized.data(), size, size, 0, STBIR_RGBA))
                return std::unexpected("Cannot resize icon image");

            int length = 0;
            std::unique_ptr<unsigned char, decltype(&std::free)> encoded(
                stbi_write_png_to_mem(resized.data(), size * 4, size, size, 4, &length), std::free);
            if (!encoded)
                return std::unexpected("Cannot encode icon PNG");

            return std::vector(encoded.get(), encoded.get() + length);
        }

    private:
        using Pixels = std::unique_ptr<stbi_uc, decltype(&stbi_image_free)>;
        Pixels pixels_;
        int size_;

        IconImage(Pixels pixels, const int size) : pixels_(std::move(pixels)), size_(size)
        {
        }
    };

    Result<void> ProductIcon::write(ProductBuild& product, std::span<const uint8_t> bytes)
    {
        auto image = IconImage::read(bytes);
        if (!image)
            return std::unexpected(image.error());

        for (const auto& variant : monokakido::icons::variants)
        {
            auto png = image->png(variant.pixels);
            if (!png)
                return std::unexpected(png.error());

            const auto path = std::string(monokakido::paths::icons) + "/" + product.name() + std::string(variant.suffix);
            if (auto written = product.writeAsset(path, *png); !written)
                return written;
        }
        return {};
    }
}
