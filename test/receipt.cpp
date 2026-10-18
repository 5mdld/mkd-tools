//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "support/receipt.hpp"

namespace MKD::test
{
    TEST(Receipts, SyntheticPurchasesRoundTripAndPreserveMetadata)
    {
        auto receipt = require(detail::PurchaseReceipt::decode(encodedReceipt(
            R"({"version":"birb-test","purchases":{},"unitedPurchases":["test.sleepy-lion"],"extra":"keep me"})")));
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
        auto reopened = require(detail::PurchaseReceipt::open(path));
        ASSERT_NE(reopened.findPurchase("test.birbs"), nullptr);
        EXPECT_EQ(reopened.findPurchase("test.birbs")->transactionId, "birb-1");
        EXPECT_EQ(reopened.metadata().version, "birb-test");
        const auto json = require(detail::openChaChaPoly(require(reopened.encode()), receiptKey));
        EXPECT_NE(std::string(json.begin(), json.end()).find("keep me"), std::string::npos);
        EXPECT_TRUE(require(receipt.activateLocalProduct("test.lion-menu")));
        EXPECT_FALSE(require(receipt.activateLocalProduct("test.lion-menu")));
        EXPECT_EQ(receipt.metadata().unitedPurchases.back(), "test.lion-menu");
        auto damaged = require(receipt.encode());
        damaged.back() ^= 1;
        EXPECT_FALSE(detail::PurchaseReceipt::decode(damaged));
    }
}
