//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

import Foundation
import CxxStdlib
import MKDReceiptCatalog

struct ReceiptItem: Identifiable, Sendable {
    let id: String
    let title: String
    let purchaseDate: String?
    let iconURL: URL?
    let hasReceipt: Bool
}

struct ReceiptSnapshot: Sendable {
    var items: [ReceiptItem]
    let missingCount: Int
}

struct ReceiptFailure: LocalizedError, Identifiable {
    var id: String { message }
    let message: String

    var errorDescription: String? { message }
}

actor ReceiptService {
    func defaultDirectory() throws -> URL {
        let result = MKDReceipt.ReceiptCatalog.defaultDataDirectory()
        guard let path = Optional(fromCxx: result.value) else {
            throw ReceiptFailure(message: String(result.error))
        }
        return URL(fileURLWithPath: String(path), isDirectory: true)
    }

    func inspect(directory: URL) throws -> ReceiptSnapshot {
        let catalog = MKDReceipt.ReceiptCatalog(std.string(directory.path))
        let result = catalog.inspect()
        guard let snapshot = Optional(fromCxx: result.value) else {
            throw ReceiptFailure(message: String(result.error))
        }
        let items = snapshot.items.map { item in
            let date = Optional(fromCxx: item.purchaseDate).map { String($0) }
            let iconPath = Optional(fromCxx: item.iconPath).map { String($0) }
            return ReceiptItem(
                id: String(item.dictionaryId),
                title: String(item.title),
                purchaseDate: date.map { String($0.prefix { $0 != "T" }) },
                iconURL: iconPath.map { URL(fileURLWithPath: $0) },
                hasReceipt: item.hasReceipt
            )
        }
        return ReceiptSnapshot(items: items, missingCount: Int(snapshot.missingCount))
    }

    func generateMissing(directory: URL) throws {
        let catalog = MKDReceipt.ReceiptCatalog(std.string(directory.path))
        let result = catalog.generateMissing()
        guard Optional(fromCxx: result.value) != nil else {
            throw ReceiptFailure(message: String(result.error))
        }
    }
}
