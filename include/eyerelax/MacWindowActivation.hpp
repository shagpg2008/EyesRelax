#pragma once

class wxFrame;

// Show a screen-sized overlay above the menu bar and Dock, independently of
// whether macOS grants the application activation. Call on the main thread.
void ShowMacBreakWindow(wxFrame& frame);
void KeepMacBreakWindowOnTop(wxFrame& frame);

// Show the pre-break reminder over the current desktop without taking focus.
void ShowMacReminderWithoutActivating(wxFrame& frame);
