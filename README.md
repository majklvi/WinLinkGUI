# WinLinkGUI

WinLinkGUI is a lightweight Windows desktop utility (pure WinAPI) for creating, deleting, and inspecting links/reparse points through a simple GUI.

## Requirements

- Windows 10 or newer
- Visual Studio Build Tools (or Visual Studio with C++ tools)
- **x64 Native Tools Command Prompt for VS** (or equivalent Developer Command Prompt)

## Build

From the repository folder, run:

```bat
cl /EHsc /DUNICODE /D_UNICODE /DWIN32_LEAN_AND_MEAN /D_CRT_SECURE_NO_WARNINGS LinkGui.cpp user32.lib gdi32.lib comctl32.lib
```

This command produces `LinkGui.exe` in the current directory.

## Run

```bat
LinkGui.exe
```

## Notes

- Some operations (especially symbolic links) may require Administrator privileges or Windows Developer Mode.
