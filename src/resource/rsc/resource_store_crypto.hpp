//
// kiwakiwaaにより 2026/01/30 に作成されました。
//

#pragma once

#include "MKD/result.hpp"

#include <array>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>
#include <vector>

namespace MKD
{
    /**
     * Decrypts .rsc file chunks from dictionaries
     * - This is a custom cipher, not a standard cryptographic algorithm
     * - The kData1 and kData2 tables are embedded in the app binary
     * - Each dictionary product uses a unique 32-byte key
     */
    class ResourceStoreCrypto
    {
    public:
        /**
         * Derives the 32-byte decryption key for encrypted RSC resources.
         *
         * 1. Compute SHA-256 of the dictionary identifier string
         * 2. XOR the hash with a rolling window into a fixed 31-byte substitution table,
         *    where the starting offset is determined by byte 7 of the hash (mod 31)
         *
         * @param dictId Dictionary identifier (e.g. "DAIJISEN2.J")
         * @return 32-byte derived key
         */
        static std::array<uint8_t, 32> deriveKey(std::string_view dictId);

        /**
         * Decrypt RSC data using HMDicEncoder::Decode algorithm
         *
         * @param encryptedData Input data (includes 4-byte checksum at end)
         * @param key 32-byte decryption key
         * @return Decrypted data on success, or error message on failure
         *
         * @note The key must match the one used during encryption, otherwise
         *       the output will be garbage data.
         */
        static Result<std::vector<uint8_t>> decrypt(std::span<const uint8_t> encryptedData,
                                                    const std::array<uint8_t, 32>& key);

        /**
         * For encrypting data (inverse HMDicEncoder::Decode for fun/testing purposes)
         */
        static std::vector<uint8_t> encrypt(std::span<const uint8_t> plaintext, const std::array<uint8_t, 32>& key);

    private:
        inline static constexpr std::array<uint8_t, 31> kSubstitutionTable = {
            0x2B, 0xDF, 0x33, 0xEE, 0x93, 0xE8, 0x68, 0x22,
            0x95, 0xD1, 0xDE, 0xCA, 0x95, 0xFA, 0x4F, 0xFB,
            0xA0, 0xB1, 0x8B, 0x4D, 0x18, 0x82, 0xB2, 0x40,
            0xAB, 0x0F, 0x50, 0xD8, 0x21, 0x30, 0x23
        };

        static std::array<uint8_t, 32> sha256(std::span<const uint8_t> data);
    };
}
