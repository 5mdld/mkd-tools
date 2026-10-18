//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "support/test_dictionary.hpp"
#include "platform/macos/chacha_poly.hpp"
#include <array>

namespace MKD::test
{
    TEST(Receipts, SyntheticPurchasesRoundTripAndPreserveMetadata)
    {
        constexpr std::array<uint8_t, 32> key{
            'r', 'H', 'D', 'M', 'g', 'l', '1', 'P', 'S', 'W', 'o', 'r', 'X', 'j', 'q', 'g',
            'j', 'S', 'v', 'o', 'u', 'C', 'L', 'x', 'w', 'a', 'n', 'W', 's', 'H', 'S', 'A'
        };
        auto receipt = require(PurchaseReceipt::decode(require(detail::sealChaChaPoly(bytes(
                R"({"version":"birb-test","purchases":{},"unitedPurchases":["test.sleepy-lion"],"extra":"keep me"})"),
            key))));
        require(receipt.addPurchase({
            .productId = "test.birbs", .transactionId = "birb-1",
            .originalTransactionId = "birb-1", .purchaseDate = "2026-10-18"
        }));
        EXPECT_FALSE(receipt.addPurchase({.productId = "test.birbs"}));
        EXPECT_TRUE(receipt.contains("test.sleepy-lion"));
        EXPECT_EQ(receipt.findPurchase("test.sleepy-lion"), nullptr);
        Workspace workspace;
        const auto path = workspace.path / "receipt";
        require(receipt.save(path));
        auto reopened = require(PurchaseReceipt::open(path));
        ASSERT_NE(reopened.findPurchase("test.birbs"), nullptr);
        EXPECT_EQ(reopened.findPurchase("test.birbs")->transactionId, "birb-1");
        EXPECT_EQ(reopened.metadata().version, "birb-test");
        const auto json = require(detail::openChaChaPoly(require(reopened.encode()), key));
        EXPECT_NE(std::string(json.begin(), json.end()).find("keep me"), std::string::npos);
        auto damaged = require(receipt.encode());
        damaged.back() ^= 1;
        EXPECT_FALSE(PurchaseReceipt::decode(damaged));
    }
}
