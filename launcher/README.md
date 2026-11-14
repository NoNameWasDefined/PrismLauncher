# `launcher/` — the launcher's C++ code

Everything that ends up in the `prismlauncher` executable lives here (Qt 6, C++20, built by
[`CMakeLists.txt`](CMakeLists.txt)). Every source file starts with a `// File overview:` comment saying what it is for;
grep for it to skim a folder:

```bash
grep -rh -A2 "File overview:" launcher/net
```

## Mental model

```
main.cpp ──► Application (global, reach it with APPLICATION->...)
              ├─ settings()        SettingsObject  (prismlauncher.cfg)            settings/
              ├─ instances()       InstanceList ──► BaseInstance ──► MinecraftInstance   ./ + minecraft/
              │                                        ├─ settings()   (instance.cfg)
              │                                        ├─ PackProfile  (mmc-pack.json: MC + loaders)
              │                                        └─ mod/resource folder models  minecraft/mod/
              ├─ accounts()        AccountList ──► MinecraftAccount (Microsoft/offline)  minecraft/auth/
              ├─ network()         QNetworkAccessManager, used through NetJob           net/
              ├─ metadataIndex()   Prism meta server (MC/loader versions)               meta/
              └─ MainWindow, InstanceWindow, dialogs, pages                             ui/

Async work = Task (tasks/). Downloads = NetJob (net/). Launching = LaunchTask + LaunchSteps (launch/).
Mod platforms (Modrinth, CurseForge, ...) = modplatform/.
```

## Folder map

| Folder | What's inside | Guide |
|---|---|---|
| `./` (top level) | `Application`, instances (`BaseInstance`, `InstanceList`), instance tasks (create/import/copy), `LaunchController`, generic helpers (`FileSystem`, `Json`, `StringUtils`, `Version`...) | this file |
| [`tasks/`](tasks) | `Task` base class + sequential/concurrent combinators | [README](tasks/README.md) |
| [`net/`](net) | HTTP: `NetJob`, `Download`, `Upload`, sinks, validators, HTTP cache | [README](net/README.md) |
| [`settings/`](settings) | `SettingsObject`, `Setting`, INI storage, override settings | [README](settings/README.md) |
| [`launch/`](launch) | `LaunchTask`, `LaunchStep`, generic launch steps, log model | [README](launch/README.md) |
| [`minecraft/`](minecraft) | `MinecraftInstance`, components/`PackProfile`, libraries, assets, launch steps, auth, skins, worlds | [README](minecraft/README.md) |
| [`minecraft/mod/`](minecraft/mod) | mods / resource packs / shaders / data packs folder models and parsers | [README](minecraft/mod/README.md) |
| [`modplatform/`](modplatform) | Modrinth, CurseForge (flame), ATLauncher, Technic, FTB, packwiz metadata | [README](modplatform/README.md) |
| [`ui/`](ui) | all windows, dialogs, pages, widgets, themes, setup wizard | [README](ui/README.md) |
| `meta/` | client for the Prism meta server (`Index` -> `VersionList` -> `Version`) | file comments |
| `java/` | find, check and download Java runtimes | file comments |
| `archive/` | read/write zip & other archives (libarchive) | file comments |
| `icons/`, `news/`, `screenshots/`, `translations/`, `tools/`, `updater/`, `logs/` | smaller features, names say it all | file comments |
| `filelink/`, `updater/prismupdater/` | separate helper executables (symlink helper, self updater) | file comments |
| `resources/` | icons, themes and other files compiled in through `.qrc` files | — |

The Java program that actually starts Minecraft is in [`../libraries/launcher`](../libraries/launcher)
(see [`../libraries/README.md`](../libraries/README.md)).

## Where the launcher keeps its data

On Windows `%APPDATA%\PrismLauncher` (or next to the exe in portable mode), on Linux `~/.local/share/PrismLauncher`:

| Path | Written by |
|---|---|
| `prismlauncher.cfg` | `APPLICATION->settings()` |
| `accounts.json` | `AccountList` |
| `instances/<id>/instance.cfg`, `mmc-pack.json`, `minecraft/` | `BaseInstance` settings, `PackProfile`, the game |
| `instances/<id>/minecraft/mods/.index/*.pw.toml` | packwiz metadata (`modplatform/packwiz`) |
| `libraries/`, `assets/`, `meta/`, `cache/` | launch update tasks, meta index, `HttpMetaCache` |
| `logs/` | the launcher's own log |

## Recipes

**Add a source file.** Create `foo/Bar.h` + `foo/Bar.cpp`, then add both to the matching `set(..._SOURCES ...)`
list in [`CMakeLists.txt`](CMakeLists.txt) (e.g. `CORE_SOURCES`, `MINECRAFT_SOURCES`; UI code goes in
`LAUNCHER_SOURCES`, Qt Designer forms in `qt_wrap_ui(LAUNCHER_UI ...)`). Files are not picked up automatically.

**Add a launcher setting.** See [`settings/README.md`](settings/README.md).

**Run code when an instance launches.** Write a `LaunchStep` and insert it in
`MinecraftInstance::createLaunchTask()` — see [`launch/README.md`](launch/README.md). For code that must run before
the account/launch machinery even starts, `Application::launch()` is the entry point (this is where the sync check is).

**Do an HTTP request.** Use a `NetJob` — see [`net/README.md`](net/README.md). Avoid blocking `QEventLoop`s on the
UI thread.

**Add a page to Settings or to the instance window.** See [`ui/README.md`](ui/README.md).

**Find what a button does.** Find its text in a `.ui` file or in `tr("...")` calls, then the matching
`on_<objectName>_triggered/clicked` slot or `connect(...)`. Main window actions are in `ui/MainWindow.cpp`.

**Translations.** UI strings go through `tr("...")` in **English**; translators provide the other languages
(`translations/`). A string written in French inside `tr()` is shown in French to everyone who has no translation for it.

## What this fork adds (vs. upstream Prism Launcher)

| Piece | Where |
|---|---|
| Sync server client (`ServerUtils`: build/upload/fetch a modpack manifest) | [`ServerCommunication.h`](ServerCommunication.h) / `.cpp` |
| Sync settings `ModpackSyncServerURL`, `AutoSyncModpack`, `ServerId`, `ServerPsswd` | `Application.cpp` (constructor) |
| Sync check before each launch | `Application::launch()` |
| Manifest upload after creating an instance; `createInstance()` returns `InstancePtr` | `InstanceCreationTask.*` and the `*CreationTask` subclasses |
| `.json` pack import (unfinished: `processJSONPack()` is empty) | `InstanceImportTask.cpp`, `ui/pages/modplatform/ImportPage.cpp` |
| `BaseInstance::rootDir()` (same as `instanceRoot()`) | `BaseInstance.h` |

Known issues are marked with `NOTE:` / `TODO:` comments in those files (`grep -rn "NOTE:" launcher/ServerCommunication.cpp`).
To limit merge conflicts when pulling from upstream (`home`), keep fork-specific code in its own files where possible
and call into it from small hooks.
