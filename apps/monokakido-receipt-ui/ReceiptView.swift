//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

import SwiftUI
import AppKit

struct ReceiptView: View {
    @StateObject private var model = ReceiptModel()

    var body: some View {
        VStack(spacing: 0) {
            if let snapshot = model.snapshot {
                List(snapshot.items) { item in
                    ReceiptRow(item: item)
                }
                .listStyle(.inset)
                .overlay {
                    if snapshot.items.isEmpty {
                        Text("No dictionaries found.")
                            .foregroundStyle(.secondary)
                    }
                }
            } else {
                VStack(spacing: 12) {
                    Image(systemName: "folder")
                        .font(.largeTitle)
                    Text("Monokakido Data Folder")
                        .font(.headline)
                    Text("Choose the Monokakido data folder.")
                        .foregroundStyle(.secondary)
                        .multilineTextAlignment(.center)
                        .textSelection(.enabled)
                    Button("Choose Folder…", action: chooseDirectory)
                        .disabled(model.isWorking)
                }
                .padding(32)
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            }

            Divider()
            HStack {
                if let snapshot = model.snapshot {
                    Text("\(snapshot.items.count) dictionaries · \(snapshot.missingCount) missing receipt records")
                        .font(.callout)
                        .foregroundStyle(.secondary)
                }
                Spacer()
                if model.isWorking {
                    ProgressView().controlSize(.small)
                }
            }
            .padding(12)
        }
        .frame(minWidth: 560, minHeight: 340)
        .toolbar {
            Button(action: chooseDirectory) {
                Label("Choose Folder…", systemImage: "folder")
            }
            .help(model.directory?.path ?? "Choose the Monokakido data folder")
            .disabled(model.isWorking)

            Button {
                Task { await model.refresh() }
            } label: {
                Label("Refresh", systemImage: "arrow.clockwise")
            }
            .keyboardShortcut("r")
            .disabled(model.isWorking || model.directory == nil)

            Button("Generate Missing") {
                Task { await model.generateMissing() }
            }
            .disabled(model.isWorking || (model.snapshot?.missingCount ?? 0) == 0)
        }
        .task { await model.start() }
        .alert(item: $model.failure) { failure in
            Alert(title: Text("Monokakido Receipt Manager"), message: Text(failure.message))
        }
    }

    private func chooseDirectory() {
        let panel = NSOpenPanel()
        panel.message = "Select the “group.jp.monokakido.Dictionaries” folder."
        panel.prompt = "Select"
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.allowsMultipleSelection = false
        panel.directoryURL = model.directory
        panel.begin { response in
            guard response == .OK, let url = panel.url else { return }
            Task { @MainActor in await model.openDirectory(url) }
        }
    }
}

private struct ReceiptRow: View {
    let item: ReceiptItem

    var body: some View {
        HStack(spacing: 10) {
            DictionaryIcon(url: item.iconURL)
                .frame(width: 24, height: 24)
            Text(item.title)
                .lineLimit(1)
                .help(item.title)
            Spacer()
            Text(item.purchaseDate ?? "—")
                .foregroundStyle(.secondary)
                .monospacedDigit()
        }
        .padding(.vertical, 6)
        .opacity(item.hasReceipt ? 1 : 0.45)
    }
}

private struct DictionaryIcon: View {
    let url: URL?

    var body: some View {
        if let url, let image = NSImage(contentsOf: url) {
            Image(nsImage: image)
                .resizable()
                .scaledToFit()
        } else {
            Image(systemName: "book.closed")
                .resizable()
                .scaledToFit()
                .foregroundStyle(.secondary)
        }
    }
}
