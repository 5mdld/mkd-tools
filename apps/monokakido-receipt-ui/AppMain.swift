//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

import SwiftUI

@main
struct ReceiptApp: App {
    var body: some Scene {
        Window("Monokakido Receipt Manager", id: "receipts") {
            ReceiptView()
        }
        .defaultSize(width: 700, height: 560)
    }
}
