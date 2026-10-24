//
// kiwakiwaaにより 2026/01/30 に作成されました。
//

#include "resource_store_crypto.hpp"
#include "binary/binary_view.hpp"
#include "crypto/resource_crypto.hpp"

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
        const uint32_t outputLength = checksum ^ detail::resource_crypto::kChecksumXor;

        if (outputLength > dataLength || outputLength > 100'000'000)
            return std::unexpected("Invalid output length derived from checksum");

        std::vector<uint8_t> output(dataLength);

        if (dataLength > 0)
            detail::resource_crypto::decryptBlocks(encryptedData.first(dataLength), output,
                                                   {key, outputLength}, outputLength);

        output.resize(outputLength);
        return output;
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

        const uint32_t checksum = outputLength ^ detail::resource_crypto::kChecksumXor;

        std::vector<uint8_t> output(dataLength + 4, 0);
        if (!plaintext.empty())
            std::memcpy(output.data(), plaintext.data(), plaintext.size());

        if (dataLength > 0)
            detail::resource_crypto::encryptBlocks(std::span(output).first(dataLength),
                                                   {key, outputLength}, outputLength);

        for (size_t i = 0; i < 4; ++i)
            output[dataLength + i] = static_cast<uint8_t>(checksum >> (8 * i));
        return output;
    }
}
