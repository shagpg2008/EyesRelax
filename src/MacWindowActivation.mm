#include "eyerelax/MacWindowActivation.hpp"

#include <wx/frame.h>
#import <AppKit/AppKit.h>

void KeepMacBreakWindowOnTop(wxFrame& frame) {
    NSWindow* window = frame.GetWXWindow();
    if (![window isVisible]) return;
    // wxSTAY_ON_TOP maps to NSModalPanelWindowLevel, below the system UI.
    [window setLevel:NSScreenSaverWindowLevel];
    [window orderFrontRegardless];
}

void ShowMacBreakWindow(wxFrame& frame) {
    NSWindow* window = frame.GetWXWindow();
    [window setHidesOnDeactivate:NO];
    [window setHasShadow:NO];
    [window setOpaque:YES];
    [window setBackgroundColor:[NSColor blackColor]];
    NSWindowCollectionBehavior behavior = [window collectionBehavior];
    behavior &= ~(NSWindowCollectionBehaviorMoveToActiveSpace |
                  NSWindowCollectionBehaviorFullScreenPrimary);
    behavior |= NSWindowCollectionBehaviorCanJoinAllSpaces |
                NSWindowCollectionBehaviorFullScreenAuxiliary;
    [window setCollectionBehavior:behavior];
    [window setLevel:NSScreenSaverWindowLevel];
    frame.Show();
    KeepMacBreakWindowOnTop(frame);

    // Request keyboard focus, but don't rely on this request being granted
    // for the overlay to cover the desktop and system UI.
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 140000
    if (@available(macOS 14.0, *)) {
        [NSApp activate];
    } else
#endif
    {
        [NSApp activateIgnoringOtherApps:YES];
    }

    [window makeKeyAndOrderFront:nil];
    KeepMacBreakWindowOnTop(frame);
}

void ShowMacReminderWithoutActivating(wxFrame& frame) {
    frame.ShowWithoutActivating();
    NSWindow* window = frame.GetWXWindow();
    // wxOSX's ShowWithoutActivating() sets hidesOnDeactivate to YES and uses
    // orderFront:, which is insufficient while another application is active.
    [window setHidesOnDeactivate:NO];
    NSWindowCollectionBehavior behavior = [window collectionBehavior];
    behavior &= ~(NSWindowCollectionBehaviorMoveToActiveSpace |
                  NSWindowCollectionBehaviorFullScreenPrimary);
    behavior |= NSWindowCollectionBehaviorCanJoinAllSpaces |
                NSWindowCollectionBehaviorFullScreenAuxiliary;
    [window setCollectionBehavior:behavior];
    // Unlike makeKeyAndOrderFront:, this does not steal keyboard focus.
    [window orderFrontRegardless];
}
