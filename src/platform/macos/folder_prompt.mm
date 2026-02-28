#include "MKD/platform/macos/folder_prompt.hpp"

#import <Cocoa/Cocoa.h>

namespace MKD::macOS
{
    void activateAsAccessory()
    {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
        [NSApp activateIgnoringOtherApps:YES];
    }

    std::optional<FolderPromptResult> promptForFolder(const FolderPromptOptions& options)
    {
        @autoreleasepool
        {
            NSOpenPanel* panel = [NSOpenPanel openPanel];
            if (!panel)
                return std::nullopt;

            panel.message = [NSString stringWithUTF8String:options.message.c_str()];
            panel.prompt  = [NSString stringWithUTF8String:options.confirmButton.c_str()];
            panel.canChooseFiles = NO;
            panel.canChooseDirectories = YES;
            panel.allowsMultipleSelection = NO;

            if (options.initialDirectory && !options.initialDirectory->empty())
            {
                NSString* path =
                    [NSString stringWithUTF8String:options.initialDirectory->c_str()];
                if (path)
                {
                    NSURL* url = [NSURL fileURLWithPath:path];
                    if (url)
                        panel.directoryURL = url;
                }
            }

            if ([panel runModal] != NSModalResponseOK)
                return std::nullopt;

            NSURL* selected = panel.URLs.firstObject;
            if (!selected || !selected.path)
                return std::nullopt;

            NSError* error = nil;
            NSData* bookmark = [selected bookmarkDataWithOptions:
                                         NSURLBookmarkCreationWithSecurityScope |
                                         NSURLBookmarkCreationSecurityScopeAllowOnlyReadAccess
                                         includingResourceValuesForKeys:nil
                                         relativeToURL:nil
                                         error:&error];

            if (!bookmark || error)
                return std::nullopt;

            const char* utf8Path = selected.path.UTF8String;
            if (!utf8Path)
                return std::nullopt;

            FolderPromptResult result;
            result.path = std::filesystem::path{utf8Path};
            result.bookmarkData.assign(
                static_cast<const uint8_t*>(bookmark.bytes),
                static_cast<const uint8_t*>(bookmark.bytes) + bookmark.length
            );

            return result;
        }
    }
}