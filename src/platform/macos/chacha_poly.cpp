//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#include "chacha_poly.hpp"

#include <CryptoKit/CryptoKit.hpp>
#include <Foundation/Foundation.hpp>

#include <cstring>

namespace MKD::detail
{
    Result<std::vector<uint8_t>> sealChaChaPoly(
        std::span<const uint8_t> plaintext, std::span<const uint8_t, 32> keyBytes)
    {
        NS::ScopedAutoreleasePool pool;
        auto* key = CK::SymmetricKey::symmetricKey(NS::Data::dataWithBytes(keyBytes.data(), keyBytes.size()));
        if (!key)
            return std::unexpected("Failed to construct ChaChaPoly key");

        const auto* box = CK::ChaChaPoly::seal(NS::Data::dataWithBytes(plaintext.data(), plaintext.size()), key);
        if (!box)
            return std::unexpected("Failed to encrypt ChaChaPoly payload");

        const auto* combined = box->combined();
        if (!combined)
            return std::unexpected("Failed to read ChaChaPoly combined bytes");

        std::vector<uint8_t> encrypted(combined->length());
        if (!encrypted.empty())
            std::memcpy(encrypted.data(), combined->bytes(), encrypted.size());
        return encrypted;
    }

    Result<std::vector<uint8_t>> openChaChaPoly(
        std::span<const uint8_t> encrypted, std::span<const uint8_t, 32> keyBytes)
    {
        if (encrypted.size() < 28)
            return std::unexpected("ChaChaPoly payload is too small");

        NS::ScopedAutoreleasePool pool;
        auto* key = CK::SymmetricKey::symmetricKey(NS::Data::dataWithBytes(keyBytes.data(), keyBytes.size()));
        if (!key)
            return std::unexpected("Failed to construct ChaChaPoly key");

        auto* box = CK::SealedBox::sealedBox(NS::Data::dataWithBytes(encrypted.data(), encrypted.size()));
        if (!box)
            return std::unexpected("Failed to parse ChaChaPoly sealed box");

        const auto* plaintext = CK::ChaChaPoly::open(box, key);
        if (!plaintext)
            return std::unexpected("Failed to authenticate ChaChaPoly payload");

        std::vector<uint8_t> decrypted(plaintext->length());
        if (!decrypted.empty())
            std::memcpy(decrypted.data(), plaintext->bytes(), decrypted.size());
        return decrypted;
    }
}
