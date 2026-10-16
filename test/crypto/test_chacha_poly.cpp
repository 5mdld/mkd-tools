//
// kiwakiwaaにより 2026/03/27 に作成されました。
//

#include <gtest/gtest.h>

#include <CryptoKit/CryptoKit.hpp>
#include <Foundation/Foundation.hpp>

#include <array>
#include <cstring>
#include <vector>

TEST(ChaChaPoly, RoundTrip)
{
    NS::ScopedAutoreleasePool pool;

    constexpr std::array<uint8_t, 32> keyBytes{0x01};
    CK::SymmetricKey* pKey = CK::SymmetricKey::symmetricKey(NS::Data::dataWithBytes(keyBytes.data(), keyBytes.size()));
    ASSERT_TRUE(pKey) << "Failed to create symmetric key";

    constexpr auto plaintext = "Hello, ChaChaPoly from C++";
    const auto plaintextLen = std::strlen(plaintext);
    NS::Data* pPlaintext = NS::Data::dataWithBytes(plaintext, plaintextLen);
    ASSERT_NE(pPlaintext, nullptr);

    CK::SealedBox* pSealedBox = CK::ChaChaPoly::seal(pPlaintext, pKey);
    ASSERT_NE(pSealedBox, nullptr) << "Encryption failed";

    NS::Data* pCombined = pSealedBox->combined();

    ASSERT_NE(pCombined, nullptr);
    // The combined representation contains a 12-byte nonce and a 16-byte tag.
    ASSERT_EQ(pCombined->length(), 12u + plaintextLen + 16u);
    const auto* combinedBytes = static_cast<const uint8_t*>(pCombined->bytes());
    EXPECT_NE(std::memcmp(combinedBytes + 12, plaintext, plaintextLen), 0);

    NS::Data* pDecrypted = CK::ChaChaPoly::open(pSealedBox, pKey);
    ASSERT_NE(pDecrypted, nullptr) << "Decryption failed";

    // roundtrip
    EXPECT_EQ(pDecrypted->length(), plaintextLen);
    EXPECT_EQ(std::memcmp(pDecrypted->bytes(), plaintext, plaintextLen), 0);
}

TEST(ChaChaPoly, RoundTripFromCombined)
{
    NS::ScopedAutoreleasePool pool;

    constexpr std::array<uint8_t, 32> keyBytes{0x01};
    CK::SymmetricKey* pKey = CK::SymmetricKey::symmetricKey(NS::Data::dataWithBytes(keyBytes.data(), keyBytes.size()));

    constexpr auto plaintext = "Reconstruct from combined bytes";
    const auto plaintextLen = std::strlen(plaintext);
    NS::Data* pPlaintext = NS::Data::dataWithBytes(plaintext, plaintextLen);

    // encrypt and get combined representation
    CK::SealedBox* pOrigBox = CK::ChaChaPoly::seal(pPlaintext, pKey);
    ASSERT_TRUE(pOrigBox);

    NS::Data* pCombined = pOrigBox->combined();

    // reconstruct sealed box from combined bytes
    CK::SealedBox* pReconstructed = CK::SealedBox::sealedBox(pCombined);
    ASSERT_TRUE(pReconstructed) << "Failed to reconstruct SealedBox from combined data";

    // decrypt the reconstructed box
    NS::Data* pDecrypted = CK::ChaChaPoly::open(pReconstructed, pKey);
    ASSERT_NE(pDecrypted, nullptr);
    EXPECT_EQ(pDecrypted->length(), plaintextLen);
    EXPECT_EQ(std::memcmp(pDecrypted->bytes(), plaintext, plaintextLen), 0);
}

TEST(ChaChaPoly, WrongKeyFails)
{
    NS::ScopedAutoreleasePool pool;

    constexpr std::array<uint8_t, 32> key1{0x01};
    constexpr std::array<uint8_t, 32> key2{0x02};
    CK::SymmetricKey* pKey1 = CK::SymmetricKey::symmetricKey(NS::Data::dataWithBytes(key1.data(), key1.size()));
    CK::SymmetricKey* pKey2 = CK::SymmetricKey::symmetricKey(NS::Data::dataWithBytes(key2.data(), key2.size()));

    constexpr auto plaintext = "Secret message";
    NS::Data* pPlaintext = NS::Data::dataWithBytes(plaintext, std::strlen(plaintext));

    CK::SealedBox* pSealedBox = CK::ChaChaPoly::seal(pPlaintext, pKey1);
    ASSERT_TRUE(pSealedBox);

    NS::Data* pDecrypted = CK::ChaChaPoly::open(pSealedBox, pKey2);
    EXPECT_EQ(pDecrypted, nullptr) << "Decryption should fail with wrong key";
}

TEST(ChaChaPoly, TamperedCombinedFails)
{
    NS::ScopedAutoreleasePool pool;

    constexpr std::array<uint8_t, 32> keyBytes{0x01};
    CK::SymmetricKey* pKey = CK::SymmetricKey::symmetricKey(NS::Data::dataWithBytes(keyBytes.data(), keyBytes.size()));
    ASSERT_TRUE(pKey);

    constexpr auto plaintext = "Tamper check";
    NS::Data* pPlaintext = NS::Data::dataWithBytes(plaintext, std::strlen(plaintext));
    ASSERT_TRUE(pPlaintext);

    CK::SealedBox* pSealedBox = CK::ChaChaPoly::seal(pPlaintext, pKey);
    ASSERT_TRUE(pSealedBox);

    NS::Data* pCombined = pSealedBox->combined();
    ASSERT_TRUE(pCombined);
    ASSERT_GT(pCombined->length(), 0u);

    std::vector<uint8_t> tampered(pCombined->length());
    std::memcpy(tampered.data(), pCombined->bytes(), pCombined->length());
    tampered.back() ^= 0x80;

    NS::Data* pTampered = NS::Data::dataWithBytes(tampered.data(), tampered.size());
    ASSERT_TRUE(pTampered);

    CK::SealedBox* pTamperedBox = CK::SealedBox::sealedBox(pTampered);
    ASSERT_TRUE(pTamperedBox) << "Tampered bytes should still parse as a SealedBox payload";

    NS::Data* pDecrypted = CK::ChaChaPoly::open(pTamperedBox, pKey);
    EXPECT_EQ(pDecrypted, nullptr) << "Tampered payload should fail authentication";
}

TEST(ChaChaPoly, MalformedCombinedRejected)
{
    NS::ScopedAutoreleasePool pool;

    // ChaChaPoly combined payload is nonce (12) + ciphertext (>=0) + tag (16).
    std::vector<uint8_t> malformed(27, 0xA5);
    NS::Data* pMalformed = NS::Data::dataWithBytes(malformed.data(), malformed.size());
    ASSERT_TRUE(pMalformed);

    CK::SealedBox* pBox = CK::SealedBox::sealedBox(pMalformed);
    EXPECT_EQ(pBox, nullptr);
}

TEST(ChaChaPoly, KeyFromData)
{
    NS::ScopedAutoreleasePool pool;

    // create key from some known bytes
    constexpr uint8_t keyBytes[32] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20
    };
    NS::Data* pKeyData = NS::Data::dataWithBytes(keyBytes, 32);
    CK::SymmetricKey* pKey = CK::SymmetricKey::symmetricKey(pKeyData);
    ASSERT_TRUE(pKey);

    constexpr auto plaintext = "Known key test";
    NS::Data* pPlaintext = NS::Data::dataWithBytes(plaintext, std::strlen(plaintext));

    CK::SealedBox* pSealedBox = CK::ChaChaPoly::seal(pPlaintext, pKey);
    ASSERT_TRUE(pSealedBox);

    NS::Data* pDecrypted = CK::ChaChaPoly::open(pSealedBox, pKey);
    ASSERT_NE(pDecrypted, nullptr);
    EXPECT_EQ(std::memcmp(pDecrypted->bytes(), plaintext, std::strlen(plaintext)), 0);
}

TEST(ChaChaPoly, LargeData)
{
    NS::ScopedAutoreleasePool pool;

    constexpr std::array<uint8_t, 32> keyBytes{0x01};
    CK::SymmetricKey* pKey = CK::SymmetricKey::symmetricKey(NS::Data::dataWithBytes(keyBytes.data(), keyBytes.size()));

    constexpr size_t dataSize = 1024 * 1024;
    std::vector<uint8_t> largeData(dataSize);
    for (size_t i = 0; i < dataSize; ++i)
        largeData[i] = static_cast<uint8_t>(i & 0xFF);

    NS::Data* pPlaintext = NS::Data::dataWithBytes(largeData.data(), dataSize);

    CK::SealedBox* pSealedBox = CK::ChaChaPoly::seal(pPlaintext, pKey);
    ASSERT_TRUE(pSealedBox);

    NS::Data* pDecrypted = CK::ChaChaPoly::open(pSealedBox, pKey);
    ASSERT_NE(pDecrypted, nullptr);
    EXPECT_EQ(pDecrypted->length(), dataSize);
    EXPECT_EQ(std::memcmp(pDecrypted->bytes(), largeData.data(), dataSize), 0);
}
