# re-won-launcher-1792

This is a source-level reconstruction of the **WON-era Half-Life launcher**, build **1792** compiled *Sep 20 2001*, protocol version **1.1.0.8**. This project aims to recereate the original launcher sources for archival & historical reasons and aims to bring various improvements and bug fixes over the original version of the launcher.

<p align="center">
  <img src="images/gallery/main_menu.png" alt="Main menu" width="480">
</p>

<table align="center" width="640">
<tr>
<td align="center"><a href="images/gallery/controls.png"><img src="images/gallery/controls.png" alt="Controls" width="150"></a><br><sub>Controls</sub></td>
<td align="center"><a href="images/gallery/advanced_controls.png"><img src="images/gallery/advanced_controls.png" alt="Advanced controls" width="150"></a><br><sub>Advanced controls</sub></td>
<td align="center"><a href="images/gallery/video_options.png"><img src="images/gallery/video_options.png" alt="Video options" width="150"></a><br><sub>Video options</sub></td>
<td align="center"><a href="images/gallery/video_modes.png"><img src="images/gallery/video_modes.png" alt="Video modes" width="150"></a><br><sub>Video modes</sub></td>
</tr>
<tr>
<td align="center"><a href="images/gallery/customize.png"><img src="images/gallery/customize.png" alt="Customize" width="150"></a><br><sub>Customize</sub></td>
<td align="center"><a href="images/gallery/internet_games.png"><img src="images/gallery/internet_games.png" alt="Internet games" width="150"></a><br><sub>Internet games</sub></td>
<td align="center"><a href="images/gallery/game_info.png"><img src="images/gallery/game_info.png" alt="Game info" width="150"></a><br><sub>Game info</sub></td>
<td align="center"><a href="images/gallery/filter_servers.png"><img src="images/gallery/filter_servers.png" alt="Filter servers" width="150"></a><br><sub>Filter servers</sub></td>
</tr>
<tr>
<td align="center"><a href="images/gallery/chat_rooms.png"><img src="images/gallery/chat_rooms.png" alt="Chat rooms" width="150"></a><br><sub>Chat rooms</sub></td>
<td align="center"><a href="images/gallery/create_game.png"><img src="images/gallery/create_game.png" alt="Create game" width="150"></a><br><sub>Create game</sub></td>
<td align="center"><a href="images/gallery/create_game_advanced.png"><img src="images/gallery/create_game_advanced.png" alt="Create game advanced options" width="150"></a><br><sub>Advanced options</sub></td>
<td align="center"><a href="images/gallery/custom_game.png"><img src="images/gallery/custom_game.png" alt="Custom game" width="150"></a><br><sub>Custom game</sub></td>
</tr>
</table>

This project has been reversed **95% by using LLMs and agent orchestration in a matter of 2 months**. By utilizing advanced agent orchestration techniques and custom-crafted prompts, it was possible to reconstruct the imaginatory style of the code the original launcher might've once had by an acceptable margin. While I personally love reversing, the introduction of LLMs and agentic workflows has helped me to create projects like this one, because the agents significantly speedup the process of reversing I would need do by hand, painfuly slowly.

While there's no build of the launcher with debug symbols we know of, there are various sources from where we can gather extra information about the code:

- RTTI for class names.
- NetGame.exe launcher for a bunch of file names.
- The fact that the linker puts source files in chronological order sorted alphabetically into the binary helps us to deduce file names more accurately.
- Quake sources, since this code has a bunch of overlap.

Using a combination of such sources we are able to guess how did the original code look like, with some accuracy.

## Fixes

This is a list of fixes this project implements:

- **More video modes resolutions**: The video modes dialog now have more resolutions available.
- **Window title bar**: In Windowed mode, the launcher is now a full-fledged window with a title bar and no new windows when selecting sub menus.
- **Working fullscreen**: In Fullscreen mode the launcher fills the screen again instead of sitting in a small window in the corner.
- **Prompts open on the launcher**: Message boxes and prompts are centred on the launcher window instead of in the middle of the screen.
- **Remembered window position**: In Windowed mode the launcher reopens where you last dragged it.
- **Mouse wheel scrolling**: The wheel now scrolls whatever list, drop-down or scrollbar the pointer is over.
- **Keyboard list navigation**: The arrow keys, Page Up/Down, Home and End move the selection in a list, and Enter opens it.

## Installation

The original launcher carries out several dependencies such as `vgui.dll`, or the WON DLLs such as `WONCrypt.dll` or `WONAuth.dll`. For easy installation, by default, we bundle these DLLs into the launcher executable itself. This is also why the executable is bigger than the original file (among other factors).

The installation is merely about copying the `hl.exe` file into the `Half-Life` folder and launching it.

### Which versions of Half-Life are supported?

Short answer: Only **Half-Life 1.1.0.8**, the final WON release from September 2001, is supported. This is the version of the game we used to reconstruct the source.

In the old versions of the game, backwards compatibility is not established due to various factors such as incompatible DLL dependencies, engine interface differences, and so on. Therefore **other versions of the game are not compatible with this source reconstruction**.

## WON Server Emulation

Part of the reconstruction was also a WON server emulation. Since some of the pages such as Chat Rooms or Internet Games are technically not easily testable because WON servers are down, an emulator has also been built which allowed me to visually confirm and test these pages.

The emulator can be found at `wonserver/`.

Quick start:

1. Build the project (see below); `wonserverd.exe` builds along with `hl.exe`.
2. Run `wonserverd.exe` once from the folder you want it to keep its keys in.
   It generates `verifier.key`, `auth.key` and `kver.kp` on first start (this
   can take about a minute).
3. Copy the generated `kver.kp` into your Half-Life folder, replacing the
   original — the launcher uses it to trust the emulator's certificates.
4. Point every block in the Half-Life folder's `woncomm.lst` at
   `127.0.0.1`. See `wonserver/README.md`
5. Keep `wonserverd.exe` running, then launch `hl.exe` as normal.

## Building

### Prerequisites

- **Visual Studio 2026** (toolset **v145**) with the following components:
  - *Desktop development with C++*
  - *C++ MFC for v145 build tools (x86 & x64)*
  - *C++ MFC for v145 build tools with Spectre mitigations* is **not** required,
    but the **MBCS** MFC libraries **are** — install
    *C++ MFC for ... (MBCS)* (the launcher is a MultiByte, not Unicode, MFC app).
  - A Windows 10/11 SDK (provides `winmm.lib`, `ddraw.lib`, `dxguid.lib`).
- **CMake ≥ 3.25**.
- **Win32 (x86)** — the entire reconstruction is 32-bit.

### Setup

```sh
cmake -S . -B build -G "Visual Studio 18 2026" -A Win32
cmake --build build --config Debug
```

If your CMake doesn't yet know the 2026 generator, use the 2022 generator with
the v145 toolset:

```sh
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 -T v145
cmake --build build --config Debug
```

Open `build/re-won-launcher-1792.sln` to develop in the IDE; `hl` is set as the
startup project.

### Deploying to a game folder

Copy `halflife_path.txt.example` to `halflife_path.txt` and point it at your
WON-era Half-Life install. After that, every build copies `hl.exe` (and the WON
DLLs) into that folder automatically, and F5 in Visual Studio launches the
deployed copy directly. Skip this file and the build still works — it just
leaves the binaries in `build/<config>/` for you to copy manually.