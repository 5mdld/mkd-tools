//
// kiwakiwaaにより 2026/01/30 に作成されました。
//

#include "resource_store_crypto.hpp"
#include "binary/binary_view.hpp"
#include "binary/binary_buffer.hpp"

#if defined(__APPLE__)
#include <CommonCrypto/CommonDigest.h>
#elif defined(_WIN32)
#include <windows.h>
#include <bcrypt.h>
#else
#include <openssl/sha.h>
#endif

#include <cstring>

namespace MKD
{
    std::array<uint8_t, 32> ResourceStoreCrypto::deriveKey(std::string_view dictId)
    {
        auto key = sha256({
            reinterpret_cast<const uint8_t*>(dictId.data()),
            dictId.size()
        });

        // Determine starting offset into the substitution table from byte 7
        const uint8_t startOffset = key[7] % 31;

        // XOR each byte of the hash with the table and wrap at 31 bytes
        size_t tableIdx = startOffset;
        for (size_t i = 0; i < 32; ++i)
        {
            key[i] ^= kSubstitutionTable[tableIdx];
            if (++tableIdx >= kSubstitutionTable.size())
                tableIdx = 0;
        }

        return key;
    }


    Result<std::vector<uint8_t>> ResourceStoreCrypto::decrypt(
        const std::span<const uint8_t> encryptedData, const std::array<uint8_t, 32>& key)
    {
        if (encryptedData.size() < 4)
            return std::unexpected("Encrypted data too short (minimum 4 bytes)");

        // Extract checksum (last 4 bytes)
        const size_t dataLength = encryptedData.size() - 4;
        if (dataLength % 16 != 0)
            return std::unexpected("Encrypted payload is not a whole permutation block");
        const auto checksum = *detail::BinaryView(encryptedData).readLittle<uint32_t>(dataLength);

        // XOR checksum with constant to get actual output length
        const uint32_t outputLength = checksum ^ kChecksumXor;

        if (outputLength > dataLength || outputLength > 100'000'000)
            return std::unexpected("Invalid output length derived from checksum");

        std::vector<uint8_t> output(dataLength);

        // Permute data using kData2 lookup table
        if (dataLength > 0)
            permuteData(encryptedData.first(dataLength), output, checksum);

        // XOR with key and kData1
        if (dataLength > 0)
            applyXorCipher(output, key, checksum);

        output.resize(outputLength);
        return output;
    }


    void ResourceStoreCrypto::permuteData(const std::span<const uint8_t> src, std::span<uint8_t> dst,
                                          const uint32_t checksum)
    {
        constexpr size_t BLOCK_SIZE = 16;
        constexpr size_t TABLE_SIZE = 31;

        uint32_t tableIndex = (checksum ^ kChecksumXor) % TABLE_SIZE;

        for (size_t offset = 0; offset < src.size(); offset += BLOCK_SIZE)
        {
            const size_t blockSize = std::min(BLOCK_SIZE, src.size() - offset);
            const auto& permutation = kData2[tableIndex];

            // Apply permutation to this block
            for (size_t i = 0; i < blockSize; ++i)
            {
                dst[offset + permutation[i]] = src[offset + i];
            }

            tableIndex = (tableIndex + 1) % TABLE_SIZE;
        }
    }


    void ResourceStoreCrypto::applyXorCipher(std::span<uint8_t> data, const std::array<uint8_t, 32>& key,
                                             const uint32_t checksum)
    {
        size_t data1Pos = (checksum ^ kChecksumXor) & 0x1F;
        size_t keyPos = 0;

        for (auto& byte : data)
        {
            constexpr size_t CIPHER_PERIOD = 32;

            byte ^= key[keyPos] ^ kData1[data1Pos];

            data1Pos = (data1Pos + 1) % CIPHER_PERIOD;
            keyPos = (keyPos + 1) % CIPHER_PERIOD;

            // Reset both when data1 wraps
            if (data1Pos == 0)
            {
                keyPos = 0;
            }
        }
    }


    std::array<uint8_t, 32> ResourceStoreCrypto::sha256(const std::span<const uint8_t> data)
    {
        std::array<uint8_t, 32> digest{};

#if defined(__APPLE__)
        CC_SHA256(data.data(), static_cast<CC_LONG>(data.size()), digest.data());
#elif defined(_WIN32)
        HCRYPTPROV hProv;
        HCRYPTHASH hHash;
        CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);
        CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash);
        CryptHashData(hHash, data.data(), data.size(), 0);
        DWORD hashLen = digest.size();
        CryptGetHashParam(hHash, HP_HASHVAL, digest.data(), &hashLen, 0);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
#else
        SHA256(data.data(), data.size(), digest.data());
#endif

        return digest;
    }


    std::vector<uint8_t> ResourceStoreCrypto::encrypt(const std::span<const uint8_t> plaintext,
                                                      const std::array<uint8_t, 32>& key)
    {
        const auto outputLength = static_cast<uint32_t>(plaintext.size());

        // pad to a multiple of 16 so every permutation block is full.
        const size_t dataLength = plaintext.empty() ? 0 : ((plaintext.size() + 15) / 16) * 16;

        const uint32_t checksum = outputLength ^ kChecksumXor;

        std::vector<uint8_t> buf(dataLength, 0);
        std::memcpy(buf.data(), plaintext.data(), plaintext.size());

        if (dataLength > 0)
            applyXorCipher(buf, key, checksum);

        std::vector<uint8_t> permuted(dataLength);
        if (dataLength > 0)
            inversePermuteData(buf, permuted, checksum);

        // append the checksum.
        detail::BinaryBuffer result(dataLength + 4);
        result.writeBytes(permuted);
        result.writeLittle(checksum);
        return result.take();
    }


    void ResourceStoreCrypto::inversePermuteData(const std::span<const uint8_t> src, std::span<uint8_t> dst,
                                                 const uint32_t checksum)
    {
        constexpr size_t BLOCK = 16, TABLE = 31;
        uint32_t ti = (checksum ^ kChecksumXor) % TABLE;

        for (size_t off = 0; off < src.size(); off += BLOCK)
        {
            const size_t bs = std::min(BLOCK, src.size() - off);
            const auto& perm = kData2[ti];
            for (size_t i = 0; i < bs; ++i)
                dst[off + i] = src[off + perm[i]];
            ti = (ti + 1) % TABLE;
        }
    }
}
