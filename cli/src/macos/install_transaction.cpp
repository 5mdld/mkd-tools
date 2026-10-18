//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "install_transaction.hpp"
#include "MKD/platform/mmap_file.hpp"
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#include <cstdio>
#include <fstream>
#include <algorithm>
#include <cerrno>

namespace MKDCLI::macOS
{
    InstallTransaction::InstallTransaction(std::filesystem::path store) : store_(std::move(store)) {}

    InstallTransaction::~InstallTransaction()
    {
        if (lock_ >= 0)
            close(lock_);
    }

    MKD::Result<void> InstallTransaction::begin(std::string_view name)
    {
        if (!std::filesystem::is_directory(store_ / "dictionaries") ||
            !std::filesystem::is_regular_file(store_ / "receipt/purchases"))
            return std::unexpected("Open Dictionaries once to initialise its data directory and receipt");
        lock_ = open((store_ / ".mkd-install.lock").c_str(), O_CREAT | O_RDWR | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (lock_ < 0 || flock(lock_, LOCK_EX | LOCK_NB) != 0)
            return std::unexpected("Cannot lock the installation directory. Another installer may be running.");

        const auto directory = store_ / ".mkd-install";
        if (!std::filesystem::create_directory(directory))
            return std::unexpected("An installation needs recovery or cleanup. See " + directory.string());
        active_ = true;
        std::filesystem::permissions(directory, std::filesystem::perms::owner_all);
        destination_ = store_ / "dictionaries" / name;
        staged_ = directory / "staged" / name;
        std::filesystem::create_directories(staged_);
        std::filesystem::copy_file(store_ / "receipt/purchases", directory / "purchases.before");
        std::ofstream instructions(directory / "RECOVERY.txt");
        instructions << "Close Dictionaries before recovery.\n"
                     << "Installed product: " << destination_.string() << "\n"
                     << "To undo replacement, move the installed product aside and restore previous if present.\n"
                     << "purchases.before is the receipt backup. Do not restore it over unrelated receipt changes.\n"
                     << "You may keep a working installation. Move this directory aside before retrying.\n";
        instructions.close();
        if (!instructions)
            return std::unexpected("Cannot write recovery instructions");
        return {};
    }

    MKD::Result<void> InstallTransaction::publish(bool replace)
    {
        const auto directory = store_ / ".mkd-install";
        auto original = MKD::MappedFile::open(directory / "purchases.before");
        auto current = MKD::MappedFile::open(store_ / "receipt/purchases");
        if (!original || !current || !std::ranges::equal(original->data(), current->data()))
            return std::unexpected("The receipt changed during installation. Nothing has been published.");

        if (std::filesystem::exists(std::filesystem::symlink_status(destination_)))
        {
            if (!replace)
                return std::unexpected("Product already installed. Use --replace to update it.");
            std::filesystem::rename(destination_, directory / "previous");
        }
        if (renameatx_np(AT_FDCWD, staged_.c_str(), AT_FDCWD, destination_.c_str(), RENAME_EXCL) != 0)
            return std::unexpected("Cannot publish dictionary: " +
                                   std::error_code(errno, std::generic_category()).message());
        published_ = true;
        return {};
    }

    MKD::Result<void> InstallTransaction::complete()
    {
        // The dictionary and activation are ready. Cleanup must never roll installation back.
        active_ = false;
        std::error_code error;
        const auto directory = store_ / ".mkd-install";
        std::filesystem::remove_all(directory, error);
        if (error)
            return std::unexpected("Installed successfully. Remove recovery files at " + directory.string());
        return {};
    }

    std::unexpected<std::string> InstallTransaction::fail(std::string error)
    {
        if (!active_)
            return std::unexpected(std::move(error));
        const auto directory = store_ / ".mkd-install";
        try
        {
            if (published_)
                std::filesystem::rename(destination_, staged_);
            if (std::filesystem::exists(directory / "previous"))
                std::filesystem::rename(directory / "previous", destination_);
            std::filesystem::remove_all(directory);
            active_ = false;
        }
        catch (const std::filesystem::filesystem_error& failure)
        {
            error += "\nRollback failed: " + std::string(failure.what()) + "\nRecovery files: " + directory.string();
        }
        return std::unexpected(std::move(error));
    }
}
