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
