#include "monokakido/core/platform/fs.hpp"

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

#include <format>
#include <fstream>
#include <sstream>

namespace monokakido::platform::fs {

    std::filesystem::path getContainerPathByGroupIdentifier(const std::string& groupIdentifier)
    {
        @autoreleasepool {
              NSString* groupId = [NSString stringWithUTF8String:groupIdentifier.c_str()];
              NSURL* containerURL = [[NSFileManager defaultManager] containerURLForSecurityApplicationGroupIdentifier:groupId];
              if (!containerURL)
                  return {};

              return std::filesystem::path{containerURL.path.UTF8String};
        }
    }

    std::expected<std::string, std::error_code> readTextFile(const std::filesystem::path& path)
    {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec))
            return std::unexpected(ec ? ec : std::make_error_code(std::errc::no_such_file_or_directory));

        if (std::filesystem::is_directory(path))
            return std::unexpected(ec ? ec : std::make_error_code(std::errc::is_a_directory));

        const auto fileSize = std::filesystem::file_size(path, ec);
        if (ec)
            return std::unexpected(ec);

        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (!file)
        {
            return std::unexpected(std::make_error_code(std::errc::io_error));
        }

        std::string content;
        content.resize(fileSize);

        if (!file.read(content.data(), static_cast<long>(fileSize)))
        {
            return std::unexpected(std::make_error_code(std::errc::io_error));
        }

        return content;
    }


    std::string makeFilestreamError(const std::ifstream& file, std::string_view context)
    {
        std::string error = std::format("Failed to {}: ", context);

        if (file.eof())
        {
            error += "unexpected end of file";
        }
        else if (file.bad())
        {
            std::error_code ec(errno, std::generic_category());
            error += std::format("system error - {}", ec.message());
        }
        else if (file.fail())
        {
            error += "format or I/O error";
        }
        else
        {
            error += "unknown error";
        }

        return error;
    }


    std::optional<BookmarkData> promptForDictionariesAccess()
    {
        @autoreleasepool {
            NSOpenPanel* panel = [NSOpenPanel openPanel];
            panel.message = @"Please select the Monokakido Dictionaries folder to grant access";
            panel.prompt = @"Grant Access";
            panel.canChooseFiles = NO;
            panel.canChooseDirectories = YES;
            panel.allowsMultipleSelection = NO;

            // Optional: try to navigate to expected location
            NSString* groupId = @"group.jp.monokakido.Dictionaries";
            NSURL* containerURL = [[NSFileManager defaultManager]
                containerURLForSecurityApplicationGroupIdentifier:groupId];
            if (containerURL) {
                NSURL* dictPath = [containerURL URLByAppendingPathComponent:
                    @"Library/Application Support/com.dictionarystore/dictionaries"];
                panel.directoryURL = dictPath;
            }

            if ([panel runModal] != NSModalResponseOK) {
                return std::nullopt;
            }

            NSURL* selectedURL = panel.URLs.firstObject;
            if (!selectedURL) {
                return std::nullopt;
            }

            // Create security-scoped bookmark
            NSError* error = nil;
            NSData* bookmark = [selectedURL bookmarkDataWithOptions:
                NSURLBookmarkCreationWithSecurityScope |
                NSURLBookmarkCreationSecurityScopeAllowOnlyReadAccess
                includingResourceValuesForKeys:nil
                relativeToURL:nil
                error:&error];

            if (!bookmark || error) {
                NSLog(@"Failed to create bookmark: %@", error);
                return std::nullopt;
            }

            BookmarkData result;
            result.data.assign(
                static_cast<const uint8_t*>(bookmark.bytes),
                static_cast<const uint8_t*>(bookmark.bytes) + bookmark.length
            );
            result.resolvedPath = std::filesystem::path{selectedURL.path.UTF8String};

            return result;
        }
    }


    // Simple preference storage using UserDefaults
    std::optional<std::vector<uint8_t>> loadSavedBookmark()
    {
        @autoreleasepool {
            NSData* data = [[NSUserDefaults standardUserDefaults]
                dataForKey:@"MonokakidoDictionariesBookmark"];

            if (!data) {
                return std::nullopt;
            }

            std::vector<uint8_t> result;
            result.assign(
                static_cast<const uint8_t*>(data.bytes),
                static_cast<const uint8_t*>(data.bytes) + data.length
            );
            return result;
        }
    }

    
    void saveBookmark(const std::vector<uint8_t>& bookmarkData)
    {
        @autoreleasepool {
            NSData* data = [NSData dataWithBytes:bookmarkData.data()
                                          length:bookmarkData.size()];
            [[NSUserDefaults standardUserDefaults]
                setObject:data forKey:@"MonokakidoDictionariesBookmark"];
        }
    }


    void clearSavedBookmark()
    {
        @autoreleasepool {
            [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"MonokakidoDictionariesBookmark"];
        }
    }
}