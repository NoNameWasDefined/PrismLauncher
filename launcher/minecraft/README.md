# `minecraft/` — everything Minecraft-specific

## The instance

`MinecraftInstance` (the only real `BaseInstance` type) ties everything together:

```
MinecraftInstance
 ├─ settings()            instance.cfg (Java, memory, window, commands... many fall back to global settings)
 ├─ getPackProfile()      PackProfile = mmc-pack.json = ordered Components
 │     └─ getProfile()    LaunchProfile = all components merged (main class, libraries, args, assets)
 ├─ loaderModList(), resourcePackList(), shaderPackList(), ...   folder models (mod/)
 ├─ worldList()           saves/ (WorldList, World)
 ├─ createUpdateTask()    update/ tasks: libraries, assets, FML libs
 └─ createLaunchTask()    launch/ steps → see ../launch/README.md
```

## Components and versions

```
mmc-pack.json ──► PackProfile ──► Component (uid + version)
                                     │ loads its VersionFile from
                                     ├─ the meta server (../meta: Index → VersionList → Version)
                                     └─ or a local patch (patches/<uid>.json, "customized" components)
ComponentUpdateTask: resolves versions + dependencies (e.g. Fabric loader → intermediary, MC → LWJGL)
VersionFile::applyTo(LaunchProfile) for each component, in order → final LaunchProfile
```

Common uids: `net.minecraft`, `org.lwjgl` / `org.lwjgl3`, `net.minecraftforge`, `net.neoforged`,
`net.fabricmc.fabric-loader`, `net.fabricmc.intermediary`, `org.quiltmc.quilt-loader`, `com.mumfrey.liteloader`.

| File | Role |
|---|---|
| `MinecraftInstance.h` | The instance: folders, models, launch command line, launch task. |
| `PackProfile.h` | Component list (with file format example in its comment). |
| `Component.h` | One component; can be disabled, moved, customized, reverted. |
| `ComponentUpdateTask.h` | Resolve component versions/dependencies online or offline. |
| `LaunchProfile.h` | Merged result used for launching. |
| `VersionFile.h`, `OneSixVersionFormat.h`, `MojangVersionFormat.h` | Version JSON data and parsers. |
| `Library.h`, `GradleSpecifier.h`, `Rule.h` | Java libraries, Maven coordinates, OS rules. |
| `AssetsUtils.h` | Asset index and asset downloads. |
| `VanillaInstanceCreationTask.h` | Creates a plain instance with a given MC version + loader. |
| `World.h`, `WorldList.h` | Worlds in `saves/`. |
| `ShortcutUtils.h` | Desktop shortcuts for instances. |

## Sub-folders

| Folder | Content |
|---|---|
| [`auth/`](auth) | Microsoft login: `AccountList` → `MinecraftAccount` → `AuthFlow` of `AuthStep`s (MSA → Xbox Live → XSTS → Minecraft token → profile). |
| [`launch/`](launch) | Minecraft launch steps (`LauncherPartLaunch` starts the game). |
| [`mod/`](mod) | Mod / resource pack / shader / data pack folders — [README](mod/README.md). |
| [`skins/`](skins) | Skin & cape management (Minecraft services API). |
| [`update/`](update) | Download tasks run before launch. |

## Recipes

Change the Minecraft version or add a loader from code:

```cpp
auto mc = std::dynamic_pointer_cast<MinecraftInstance>(instance);
auto profile = mc->getPackProfile();
profile->setComponentVersion("net.minecraft", "1.20.1", true);
profile->setComponentVersion("net.fabricmc.fabric-loader", "0.15.11");
profile->saveNow();
// dependencies (intermediary, LWJGL) are resolved on the next reload/launch
```

List installed mods with their metadata:

```cpp
auto mods = mc->loaderModList();
mods->update();                                   // async; wait for updateFinished()
for (auto* mod : mods->allMods()) { mod->name(); mod->version(); mod->metadata(); }
```
