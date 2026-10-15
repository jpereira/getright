// SPDX-License-Identifier: LGPL-3.0-only
// Copyright (C) 2026 Jorge Pereira
// Author: Jorge Pereira <jpereiran@gmail.com>
// Date: 2026-10-14
// Description: Native macOS application activation.

#import <AppKit/AppKit.h>

void activateMacApplication()
{
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
    NSWindow *window = [NSApp mainWindow];
    if (!window) {
        for (NSWindow *candidate in [NSApp orderedWindows]) {
            if ([candidate canBecomeMainWindow]) {
                window = candidate;
                break;
            }
        }
    }
    [window deminiaturize:nil];
    [window makeKeyAndOrderFront:nil];
}
