//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "install.hpp"
#include "install_transaction.hpp"
#include "MKDCLI/commands.hpp"
#include "dictionary/package/property_list.hpp"
#include "dictionary/package/native_format.hpp"
#include "MKD/platform/directory_dictionary_source.hpp"
#include "MKD/platform/macos/paths.hpp"
#include "receipt/purchase_receipt.hpp"
#include <libproc.h>
#include <algorithm>
#include <set>
#include <iostream>

namespace MKDCLI::macOS
{
    MKD::Result<void> requireDictionariesClosed()
    {
        const auto capacity = proc_listallpids(nullptr, 0);
        if (capacity <= 0)
            return std::unexpected("Cannot check whether Dictionaries is running");
        std::vector<pid_t> pids(static_cast<size_t>(capacity) + 32);
        const auto count = proc_listallpids(pids.data(), static_cast<int>(pids.size() * sizeof(pid_t)));
        if (count <= 0)
            return std::unexpected("Cannot check whether Dictionaries is running");
        for (auto pid : std::span(pids).first(std::min(static_cast<size_t>(count), pids.size())))
        {
            char path[PROC_PIDPATHINFO_MAXSIZE];
            if (proc_pidpath(pid, path, sizeof(path)) > 0 &&
                std::string_view(path).ends_with("/Contents/MacOS/Dictionaries"))
                return std::unexpected("Quit Dictionaries before installing a dictionary");
        }
        return {};
    }

    MKD::Result<MKD::DictionaryMetadata> readPackage(const std::filesystem::path& root)
    {
        using namespace MKD::detail;
        auto info = PropertyList::open(root / native::paths::productInfo, "dict");
        if (!info ||
            info->dictionary().integer(metadata::package::mkdPackageVersion) != native::defaults::mkdPackageVersion)
            return std::unexpected("Not a supported mkd package. Rebuild it with the current ProductWriter.");
        auto product = MKD::DictionaryMetadata::loadFromPath(root / native::paths::productContents /
                                                             (root.filename().string() + ".json"));
        if (!product)
            return std::unexpected(product.error());
        const auto expectedId = std::string(native::defaults::productIdentifierPrefix) + root.filename().string();
        if (product->productIdentifier != expectedId ||
            info->dictionary().text(metadata::package::identifier) != expectedId || product->contents.empty())
            return std::unexpected("Package identity does not match its metadata");
        for (const auto& content : product->contents)
            if (!content.dictionaryId || !*content.dictionaryId)
                return std::unexpected("A numeric dictionary ID is required for installation");
        return product;
    }

    MKD::Result<void> checkReplacement(const std::filesystem::path& source, const std::filesystem::path& destination,
                                       std::string_view productId, bool replace)
    {
        if (!std::filesystem::exists(std::filesystem::symlink_status(destination)))
            return {};
        if (!replace)
            return std::unexpected("Product already installed. Use --replace to update it.");
        if (std::filesystem::equivalent(source, destination))
            return std::unexpected("The supplied package is already in the installation directory");
        if (std::filesystem::is_symlink(std::filesystem::symlink_status(destination)))
            return std::unexpected("Cannot replace a symbolic link");
        auto installed = readPackage(destination);
        if (!installed || installed->productIdentifier != productId)
            return std::unexpected("Cannot replace a product that is not a matching mkd package");
        return {};
    }

    MKD::Result<void> checkConflicts(const MKD::DictionaryMetadata& product, const std::filesystem::path& dictionaries,
                                     const std::filesystem::path& destination)
    {
        std::set<uint16_t> ids;
        std::set<std::string> identifiers;
        for (const auto& content : product.contents)
        {
            ids.insert(*content.dictionaryId);
            identifiers.insert(content.identifier);
        }
        auto installed = MKD::DirectoryDictionarySource(dictionaries).findAllAvailable();
        if (!installed)
            return std::unexpected(installed.error());
        for (const auto& item : *installed)
        {
            if (std::filesystem::exists(destination) && std::filesystem::equivalent(item.path, destination))
                continue;
            auto other = MKD::DictionaryMetadata::loadFromPath(item.path / MKD::detail::native::paths::productContents /
                                                               (item.id + ".json"));
            if (!other)
                return std::unexpected(other.error());
            if (other->productIdentifier == product.productIdentifier)
                return std::unexpected("Product identifier is already installed at " + item.path.string());
            for (const auto& content : other->contents)
                if ((content.dictionaryId && ids.contains(*content.dictionaryId)) ||
                    identifiers.contains(content.identifier))
                    return std::unexpected("Dictionary identifier conflicts with " + item.path.string());
        }
        return {};
    }

    MKD::Result<std::filesystem::path> installPackage(const std::filesystem::path& package,
                                                      const std::filesystem::path& dataDirectory, bool replace)
    {
        const auto dictionaries = MKD::macOS::dictionariesDirectory(dataDirectory);
        const auto receiptPath = dictionaries.parent_path() / "receipt/purchases";
        InstallTransaction installation(dictionaries.parent_path());
        try
        {
            const auto source = std::filesystem::canonical(package);
            const auto destination = dictionaries / source.filename();
            auto product = readPackage(source);
            if (!product)
                return std::unexpected(product.error());
            if (auto started = installation.begin(source.filename().string()); !started)
                return installation.fail(started.error());
            if (auto allowed = checkReplacement(source, destination, product->productIdentifier, replace); !allowed)
                return installation.fail(allowed.error());
            if (auto conflicts = checkConflicts(*product, dictionaries, destination); !conflicts)
                return installation.fail(conflicts.error());
            auto receipt = MKD::detail::PurchaseReceipt::open(receiptPath);
            if (!receipt)
                return installation.fail(receipt.error());
            auto activated = receipt->activateLocalProduct(product->productIdentifier);
            if (!activated)
                return installation.fail(activated.error());
            std::filesystem::copy(source, installation.stagedPackage(),
                                  std::filesystem::copy_options::recursive |
                                      std::filesystem::copy_options::copy_symlinks);
            if (auto published = installation.publish(replace); !published)
                return installation.fail(published.error());
            if (*activated)
                if (auto saved = receipt->save(receiptPath); !saved)
                    return installation.fail(saved.error());
            if (auto cleaned = installation.complete(); !cleaned)
                std::cerr << "Warning: " << cleaned.error() << "\n";
            return destination;
        }
        catch (const std::exception& error)
        {
            return installation.fail(error.what());
        }
    }
}

namespace MKDCLI
{
    int runCommand(const InstallCommand& command)
    {
        if (auto closed = macOS::requireDictionariesClosed(); !closed)
        {
            std::cerr << "Error: " << closed.error() << "\n";
            return 1;
        }
        auto directory = MKD::macOS::monokakidoDataDirectory();
        if (!directory)
        {
            std::cerr << "Error: " << directory.error() << "\n";
            return 1;
        }
        auto installed = macOS::installPackage(command.package, *directory, command.replace);
        if (!installed)
        {
            std::cerr << "Error: " << installed.error() << "\n";
            return 1;
        }
        std::cout << "Installed: " << installed->string() << "\nOpen Dictionaries to use it.\n";
        return 0;
    }
}
