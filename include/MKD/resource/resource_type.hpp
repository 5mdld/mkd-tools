//
// kiwakiwaaにより 2026/02/21 に作成されました。
//

#pragma once

#include <cstdint>
#include <format>
#include <string_view>
#include <utility>


namespace MKD
{
    enum class ResourceType : uint8_t
    {
        Audio, // .nrsc
        AudioLegacy, // .rsc
        Contents,
        Graphics,
        Fonts,
        Keystores,
        Headlines
    };

    struct ResourceTypeTraits
    {
        std::string_view name;
        std::string_view blobPrefix;
        std::string_view extension;
        uint32_t seqWidth; // zero-pad width for sequence numbers
        uint32_t firstSequence; // starting sequence number

        [[nodiscard]] std::string blobFilename(uint32_t seq) const
        {
            switch (seqWidth)
            {
                case 4:  return std::format("{}{:04}{}", blobPrefix, seq, extension);
                case 5:  return std::format("{}{:05}{}", blobPrefix, seq, extension);
                default: return std::format("{}{}{}", blobPrefix, seq, extension);
            }
        }
    };

    constexpr ResourceTypeTraits resourceTraits(const ResourceType type)
    {
        switch (type)
        {
            case ResourceType::Audio:
                return {
                    .name          = "Audio",
                    .blobPrefix    = "",
                    .extension     = ".nrsc",
                    .seqWidth      = 5,
                    .firstSequence = 0
                };
            case ResourceType::AudioLegacy:
                return {
                    .name          = "Audio",
                    .blobPrefix    = "audio-",
                    .extension     = ".rsc",
                    .seqWidth      = 4,
                    .firstSequence = 1
                };
            case ResourceType::Contents:
                return {
                    .name          = "Contents",
                    .blobPrefix    = "contents-",
                    .extension     = ".rsc",
                    .seqWidth      = 4,
                    .firstSequence = 1
                };
            case ResourceType::Graphics:
                return {
                    .name          = "Graphics",
                    .blobPrefix    = "",
                    .extension     = ".nrsc",
                    .seqWidth      = 5,
                    .firstSequence = 0
                };
            case ResourceType::Fonts:
                return {
                    .name          = "Fonts",
                    .blobPrefix    = "font-",
                    .extension     = ".rsc",
                    .seqWidth      = 4,
                    .firstSequence = 1
                };
            case ResourceType::Keystores:
                return {
                    .name          = "Keystores",
                    .blobPrefix    = "",
                    .extension     = ".keystore",
                    .seqWidth      = 5,
                    .firstSequence = 0
                };
            case ResourceType::Headlines:
                return {
                    .name          = "Headlines",
                    .blobPrefix    = "",
                    .extension     = ".headlinestore",
                    .seqWidth      = 5,
                    .firstSequence = 0
                };
        }
        std::unreachable();
    }

    constexpr std::string_view resourceTypeName(const ResourceType type)
    {
        return resourceTraits(type).name;
    }
}
