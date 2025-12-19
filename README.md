# rncw (Rename Current Workspace)

A lightweight, high-performance C utility designed to rename the currently active workspace in X11 environments (Openbox, LXQt, Fluxbox, etc.).

## The Problem
Most CLI tools like `xprop` handle workspace names as standard strings. When dealing with a large number of workspaces (e.g., 40), these tools often fail to correctly insert the **Null bytes (`\0`)** required by the EWMH specification to separate names. This results in the "Panel Stretch" bug, where the entire list of names is concatenated into the first workspace's label.

`rncw` fixes this by communicating directly with the X Server, performing memory-level surgery on the `_NET_DESKTOP_NAMES` property to ensure names remain individual and correctly indexed.

---

## Building for X11 (Linux/BSD)

### 1. Dependencies
You need the X11 development libraries installed on your system.
* **Arch Linux:** `sudo pacman -S libx11 gcc`
* **Debian/Ubuntu:** `sudo apt install libx11-dev gcc`
* **Fedora:** `sudo dnf install libX11-devel gcc`

### 2. Compilation
Compile the source using `gcc`:
```bash
gcc -O2 -o rncw rncw.c -lX11
```

### 3. Installation
Move the binary to your path for global access:
```bash
sudo mv rncw /usr/local/bin/
```

---

## Usage

### CLI Mode
To rename the workspace you are currently sitting on:
```bash
rncw "Development"
```

### Rofi Integration (The "GG" Setup)
To use `rncw` with a graphical prompt, use this Zsh wrapper. It extracts the current name, opens a pre-filled Rofi prompt, and feeds the result to `rncw`.

```zsh
#!/usr/bin/env zsh
idx=$(xdotool get_desktop)
current_name=$(xprop -root _NET_DESKTOP_NAMES | perl -ne 'print "$1\n" while /"(.*?)"/g' | sed -n "$((idx + 1))p")
new_name=$(echo "" | rofi -dmenu -p "Rename:" -filter "$current_name" -lines 0)

if [[ -n "$new_name" ]]; then
    rncw "$new_name"
fi
```

---

## Wayland Support
**Important:** The current C implementation uses `Xlib`, which is specific to **X11**.

Wayland does not have a global "Root Window" property system like X11. Workspace management on Wayland is handled by the specific compositor. If you migrate to Wayland, you do not need this C tool; instead, use the native IPC commands:

* **Sway:** `swaymsg rename workspace to "Name"`
* **Hyprland:** `hyprctl dispatch renameworkspace <id> "Name"`
* **River:** `riverctl set-repeat` (depends on tags)

For X11 "Daily Drivers," `rncw` remains the most robust solution for maintaining clean panels and workspace organization.

---

## Technical Details
`rncw` performs the following steps:
1. Queries `_NET_NUMBER_OF_DESKTOPS` to safely allocate memory.
2. Queries `_NET_CURRENT_DESKTOP` to find your location.
3. Retrieves the binary blob from `_NET_DESKTOP_NAMES`.
4. Reconstructs a new buffer, replacing only the segment belonging to the current index.
5. Uses `XChangeProperty` with `PropModeReplace` to update the X Server.
```
