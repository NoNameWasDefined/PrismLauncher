# `minecraft/mod/` — resource folders (mods, packs, shaders...)

Each folder of an instance that holds user content has a Qt model that the instance pages display:

| Folder in the game dir | Model | Item class | Page (`ui/pages/instance/`) |
|---|---|---|---|
| `mods/` (+ `coremods/`, `nilmods/`) | `ModFolderModel` | `Mod` | `ModFolderPage` |
| `resourcepacks/` | `ResourcePackFolderModel` | `ResourcePack` | `ResourcePackPage` |
| `texturepacks/` (pre-1.6) | `TexturePackFolderModel` | `TexturePack` | `TexturePackPage` |
| `shaderpacks/` | `ShaderPackFolderModel` | `ShaderPack` | `ShaderPackPage` |
| `datapacks/` | `DataPackFolderModel` | `DataPack` | `DataPackPage` |

All models derive from **`ResourceFolderModel`**, which:

- watches the folder (`startWatching()`) and rescans on change (`update()` → background `ResourceFolderLoadTask`,
  then `updateFinished()`);
- parses each file in the background (`tasks/Local*ParseTask`: reads `fabric.mod.json`, `mods.toml`, `pack.mcmeta`...);
- enables/disables files (disabled = renamed with `.disabled`), installs, uninstalls and deletes them;
- attaches **platform metadata** (`Metadata::ModStruct`) read from `mods/.index/*.pw.toml`
  (see [`../../modplatform/packwiz`](../../modplatform/packwiz)) — that's how the launcher knows a mod came from
  Modrinth/CurseForge and can update it.

Get a model from the instance: `mc->loaderModList()`, `mc->resourcePackList()`, ... Typed accessors come from the
`RESOURCE_HELPERS(T)` macro: `allMods()`, `at(i)`, `selectedMods(indexes)`.

| File | Role |
|---|---|
| `Resource.h` | Base item: file, name, enabled, size, metadata. |
| `Mod.h`, `ModDetails.h` | A mod and what its jar declares (id, version, authors, loaders, license). |
| `MetadataHandler.h` | Create/read/delete the `.pw.toml` metadata of a resource. |
| `ResourceFolderModel.h` | The generic model (most logic is here). |
| `tasks/LocalModParseTask.h` | Jar metadata parsing for every loader format. |
| `tasks/GetModDependenciesTask.h` | Finds dependencies to download along with selected mods. |

Downloading a new resource is done by `ResourceDownloadTask` (`launcher/ResourceDownloadTask.h`), started from the
download dialog (`ui/dialogs/ResourceDownloadDialog.h`); it writes the file and its metadata, then the folder model
picks it up.
