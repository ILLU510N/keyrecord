#include "platform/tray.h"

#import <AppKit/AppKit.h>

#include <utility>

namespace {

keyrecord::TrayExitCallback exitCallback;
NSStatusItem* statusItem = nil;

} // namespace

@interface KeyRecordTrayTarget : NSObject
- (void)exitSelected:(id)sender;
@end

namespace {

KeyRecordTrayTarget* trayTarget = nil;

} // namespace

@implementation KeyRecordTrayTarget
- (void)exitSelected:(id)sender {
    (void)sender;
    if (exitCallback) {
        exitCallback();
    }
}
@end

namespace keyrecord {

bool initializeTray(TrayExitCallback callback) {
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

        exitCallback = std::move(callback);
        trayTarget = [[KeyRecordTrayTarget alloc] init];
        statusItem = [[NSStatusBar systemStatusBar]
            statusItemWithLength:NSSquareStatusItemLength];
        if (!statusItem || !statusItem.button) {
            shutdownTray();
            return false;
        }

        statusItem.button.toolTip = @"KeyRecord Running";
        if (@available(macOS 11.0, *)) {
            NSImage* icon = [NSImage
                imageWithSystemSymbolName:@"keyboard"
                accessibilityDescription:@"KeyRecord"];
            [icon setTemplate:YES];
            statusItem.button.image = icon;
        } else {
            statusItem.button.title = @"KR";
        }

        NSMenu* menu = [[NSMenu alloc] initWithTitle:@"KeyRecord"];
        NSMenuItem* exitItem = [[NSMenuItem alloc]
            initWithTitle:@"Exit"
            action:@selector(exitSelected:)
            keyEquivalent:@"q"];
        exitItem.target = trayTarget;
        [menu addItem:exitItem];
        statusItem.menu = menu;
        return true;
    }
}

void shutdownTray() {
    @autoreleasepool {
        if (statusItem) {
            [[NSStatusBar systemStatusBar] removeStatusItem:statusItem];
        }
        statusItem = nil;
        trayTarget = nil;
        exitCallback = {};
    }
}

} // namespace keyrecord
