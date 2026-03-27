import CryptoKit
import Foundation

// MARK: - SymmetricKey wrapper

@objc(CKSymmetricKey) public class CKSymmetricKey: NSObject {
    let key: SymmetricKey

    @objc public init(bitCount: Int) {
        self.key = SymmetricKey(size: .init(bitCount: bitCount))
        super.init()
    }

    @objc public init(data: NSData) {
        self.key = SymmetricKey(data: data as Data)
        super.init()
    }

    @objc public var rawData: NSData {
        return key.withUnsafeBytes { Data($0) } as NSData
    }
}

// MARK: - SealedBox wrapper

@objc(CKSealedBox) public class CKSealedBox: NSObject {
    let box: ChaChaPoly.SealedBox

    init(box: ChaChaPoly.SealedBox) {
        self.box = box
        super.init()
    }

    @objc public init?(combined: NSData) {
        guard let box = try? ChaChaPoly.SealedBox(combined: combined as Data) else { return nil }
        self.box = box
        super.init()
    }

    @objc public var combined: NSData {
        return box.combined as NSData
    }

    @objc public var nonce: NSData {
        return Data(box.nonce) as NSData
    }

    @objc public var ciphertext: NSData {
        return box.ciphertext as NSData
    }

    @objc public var tag: NSData {
        return box.tag as NSData
    }
}

// MARK: - ChaChaPoly operations

@objc(CKChaChaPoly) public class CKChaChaPoly: NSObject {
    @objc public static func seal(_ data: NSData, using key: CKSymmetricKey) -> CKSealedBox? {
        guard let box = try? ChaChaPoly.seal(data as Data, using: key.key) else { return nil }
        return CKSealedBox(box: box)
    }

    @objc public static func open(_ sealedBox: CKSealedBox, using key: CKSymmetricKey) -> NSData? {
        guard let data = try? ChaChaPoly.open(sealedBox.box, using: key.key) else { return nil }
        return data as NSData
    }
}

// force symbol reference from C++ so the static library always links this bridge unit.
@_cdecl("CKBridgeInitialize")
public func CKBridgeInitialize() {
    _ = CKSymmetricKey.self
    _ = CKSealedBox.self
    _ = CKChaChaPoly.self
}
