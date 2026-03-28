# Machine Learning

[简体中文](./README-zh_CN.md) | [English](./README.md)

![Preview](https://github.com/MikuTrive/MachineLearning/blob/main/Images/1.png)

![Preview](https://github.com/MikuTrive/MachineLearning/blob/main/Images/2.png)

**Machine Learning** is a native Linux desktop teaching application for the **Machine** programming language, written in **C** with **GTK4**, **GtkSourceView 5**, and **SQLite3**.

This first scaffold already implements:

- Linux-only desktop packaging
- First-launch onboarding wizard
- Instant English / Simplified Chinese interface switching
- User-selected database directory
- Resume-from-last-node persistence via SQLite
- A persistent main window named **List1**
- Plain `.mne` editing with line numbers and lesson-aware run/check workflow
- Install / uninstall targets that do **not** depend on the source tree after installation
- Isolation from the Machine compiler runtime paths under `/usr/local/lib/machine` and `/usr/local/include/machine`

## Design Notes

- The wizard is fixed-size and non-resizable.
- The main window is the long-lived workspace window called **List1**.
- If the selected database directory is deleted later, the application returns to the onboarding wizard on the next launch.
- The app installs its own resources into `/usr/local/share/machine-learning` and does **not** overwrite or relocate the Machine compiler runtime.
- The code editor keeps plain text editing behavior and does not replace or modify the Machine compiler installation.

## Window Size Mapping

GTK windows use pixel sizes, while the original product brief described window sizes as `100x35` and `145x45`.

This scaffold maps those layout units to practical desktop defaults:

- onboarding wizard: **1000 x 700**
- List1 main window: **1450 x 900**

If you want a different mapping later, it is easy to change.

## Dependencies

### Fedora

```bash
sudo dnf install -y gcc make gtk4-devel gtksourceview5-devel sqlite-devel desktop-file-utils
```

### Debian / Ubuntu

```bash
sudo apt install -y build-essential libgtk-4-dev libgtksourceview-5-dev libsqlite3-dev desktop-file-utils
```

### Arch Linux

```bash
sudo pacman -S --needed base-devel gtk4 gtksourceview5 sqlite desktop-file-utils
```

## Build Commands

Running plain `make` prints the English build parameter summary:

```bash
make
```

Build the application:

```bash
make build
```

Run it from the source tree:

```bash
make run
```

Install it system-wide:

```bash
sudo make install
```

Uninstall it:

```bash
sudo make uninstall
```

## Installed Layout

After installation, the project no longer depends on the source tree.

```text
/usr/local/bin/machine-learning
/usr/local/share/machine-learning/css/app.css
/usr/local/share/machine-learning/language-specs/machine.lang
/usr/local/share/applications/machine-learning.desktop
/usr/local/share/icons/hicolor/scalable/apps/machine-learning.svg
```

## Configuration and Database

User configuration is stored at:

```text
~/.config/machine-learning/config.ini
```

The actual lesson database is stored inside the folder selected by the user during onboarding:

```text
<selected-folder>/machine_learning.db
```

## What Works Right Now

- first launch language chooser
- immediate UI language switching
- database folder selection
- SQLite bootstrap
- persistence of the current lesson node
- automatic resume to the previous node
- List1 main workspace opening directly on later launches

## What Comes Next

This scaffold is ready for the next phase:

- full course graph
- rich lesson rendering
- Machine code execution integration
- progress checkpoints and pass/fail tracking
- more advanced editor tooling
- deeper HTML/CSS-like card layouts and learning modules
