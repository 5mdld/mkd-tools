//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

import Foundation
import Combine

@MainActor
final class ReceiptModel: ObservableObject {
    @Published private(set) var directory: URL?
    @Published private(set) var snapshot: ReceiptSnapshot?
    @Published private(set) var isWorking = false
    @Published var failure: ReceiptFailure?

    private let service = ReceiptService()
    private let defaults: UserDefaults
    private var hasStarted = false
    private static let directoryKey = "MonokakidoDataDirectory"

    init(defaults: UserDefaults = .standard) {
        self.defaults = defaults
    }

    func start() async {
        guard !hasStarted else { return }
        hasStarted = true
        isWorking = true
        defer { isWorking = false }
        do {
            let initialDirectory: URL
            if let saved = defaults.string(forKey: Self.directoryKey) {
                initialDirectory = URL(fileURLWithPath: saved, isDirectory: true)
            } else {
                initialDirectory = try await service.defaultDirectory()
            }
            try await open(initialDirectory)
        } catch {
            failure = ReceiptFailure(message: error.localizedDescription)
        }
    }

    func openDirectory(_ url: URL) async {
        guard !isWorking else { return }
        isWorking = true
        defer { isWorking = false }
        do {
            try await open(url)
        } catch {
            failure = ReceiptFailure(message: error.localizedDescription)
        }
    }

    func refresh() async {
        guard !isWorking, let directory else { return }
        isWorking = true
        defer { isWorking = false }
        do {
            try await loadSnapshot(directory: directory)
        } catch {
            failure = ReceiptFailure(message: error.localizedDescription)
        }
    }

    func generateMissing() async {
        guard !isWorking, let directory, let snapshot, snapshot.missingCount > 0 else { return }
        isWorking = true
        defer { isWorking = false }
        do {
            try await service.generateMissing(directory: directory)
            try await loadSnapshot(directory: directory)
        } catch {
            failure = ReceiptFailure(message: error.localizedDescription)
        }
    }

    private func open(_ url: URL) async throws {
        try await loadSnapshot(directory: url)
        directory = url
        defaults.set(url.path, forKey: Self.directoryKey)
    }

    private func loadSnapshot(directory: URL) async throws {
        var loaded = try await service.inspect(directory: directory)
        loaded.items.sort {
            if $0.hasReceipt != $1.hasReceipt { return $0.hasReceipt }
            return $0.title.localizedStandardCompare($1.title) == .orderedAscending
        }
        snapshot = loaded
    }
}
