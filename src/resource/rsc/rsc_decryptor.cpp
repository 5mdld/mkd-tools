//
// kiwakiwaaにより 2026/01/30 に作成されました。
//

#include "monokakido/resource/rsc/rsc_decryptor.hpp"


namespace monokakido
{
    std::expected<std::vector<uint8_t>, std::string> RscDecryptor::decrypt(
        const std::span<const uint8_t> encryptedData, const std::array<uint8_t, 32>& key)
    {
        // Validate input
        if (encryptedData.size() < 4)
        {
            return std::unexpected("Encrypted data too short (minimum 4 bytes)");
        }

        // Extract checksum (last 4 bytes)
        const size_t dataLength = encryptedData.size() - 4;
        uint32_t checksum;
        std::memcpy(&checksum, encryptedData.data() + dataLength, sizeof(uint32_t));

        // XOR checksum with constant to get actual output length
        const uint32_t outputLength = checksum ^ CHECKSUM_XOR;

        // validate output length
        if (outputLength > dataLength || outputLength > 100'000'000)
        {
            return std::unexpected("Invalid output length derived from checksum");
        }

        std::vector<uint8_t> output(dataLength);

        // Permute data using DATA2 lookup table
        if (dataLength > 0)
        {
            permuteData(encryptedData.first(dataLength), output, checksum);
        }

        // XOR with key and DATA1
        if (dataLength > 0)
        {
            applyXorCipher(output, key, checksum);
        }

        output.resize(outputLength);
        return output;
    }


    void RscDecryptor::permuteData(const std::span<const uint8_t> src, std::span<uint8_t> dst, const uint32_t checksum)
    {
        constexpr size_t BLOCK_SIZE = 16;
        constexpr size_t TABLE_SIZE = 31;

        uint32_t tableIndex = (checksum ^ CHECKSUM_XOR) % TABLE_SIZE;

        for (size_t offset = 0; offset < src.size(); offset += BLOCK_SIZE)
        {
            const size_t blockSize = std::min(BLOCK_SIZE, src.size() - offset);
            const auto& permutation = DATA2[tableIndex];

            // Apply permutation to this block
            for (size_t i = 0; i < blockSize; ++i)
            {
                dst[offset + permutation[i]] = src[offset + i];
            }

            tableIndex = (tableIndex + 1) % TABLE_SIZE;
        }
    }


    void RscDecryptor::applyXorCipher(std::span<uint8_t> data, const std::array<uint8_t, 32>& key, const uint32_t checksum)
    {
        size_t data1Pos = (checksum ^ CHECKSUM_XOR) & 0x1F;
        size_t keyPos = 0;

        for (auto& byte : data)
        {
            constexpr size_t CIPHER_PERIOD = 32;

            byte ^= key[keyPos] ^ DATA1[data1Pos];

            data1Pos = (data1Pos + 1) % CIPHER_PERIOD;
            keyPos = (keyPos + 1) % CIPHER_PERIOD;

            // Reset both when data1 wraps
            if (data1Pos == 0)
            {
                keyPos = 0;
            }
        }
    }
}
