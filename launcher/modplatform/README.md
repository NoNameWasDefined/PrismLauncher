# `modplatform/` — Modrinth, CurseForge and other platforms

Two jobs live here: **installing modpacks** (create an instance from a pack) and **talking to mod APIs** (search,
download and update individual mods/resource packs/shaders).

## Shared model

| File | Role |
|---|---|
| `ModIndex.h` | Platform-neutral types (`ModPlatform::IndexedPack` = a project, `IndexedVersion` = a file, `Dependency`, loaders, sides, hash types). |
| `ResourceAPI.h` | Interface implemented by `ModrinthAPI` and `FlameAPI`: search, project info, versions, dependencies. |
| `CheckUpdateTask.h` | Base "check these resources for updates" task. |
| `EnsureMetadataTask.h` | Hash local files and look them up on Modrinth/CurseForge to create missing metadata. |
| `packwiz/Packwiz.h` | Read/write `mods/.index/<slug>.pw.toml` metadata (what the sync manifest is built from). |
| `helpers/` | Hashing (incl. CurseForge murmur2), override tracking, mod list export. |

## Per platform

| Folder | Pack install | Mod API |
|---|---|---|
| `modrinth/` | `ModrinthCreationTask` (`.mrpack`: `modrinth.index.json` + overrides) | `ModrinthAPI`, `ModrinthCheckUpdate`, `ModrinthPackIndex` (JSON → `IndexedPack`) |
| `flame/` (CurseForge) | `FlameCreationTask` (`manifest.json` + overrides; `FileResolvingTask` resolves file ids; blocked mods → `BlockedModsDialog`) | `FlameAPI`, `FlameCheckUpdate`, `FlameModIndex` |
| `atlauncher/` | `ATLPackInstallTask` | — |
| `technic/` | `SingleZipPackInstallTask`, `SolderPackInstallTask`, `TechnicPackProcessor` | — |
| `legacy_ftb/`, `import_ftb/` | legacy FTB packs; import from the FTB App | — |

Exports the other way round: `ModrinthPackExportTask`, `FlamePackExportTask`.

## How a modpack becomes an instance

```
NewInstanceDialog page (ui/pages/modplatform/modrinth/ModrinthPage, flame/FlamePage, ImportPage...)
  └─► InstanceImportTask (download/extract pack, detect type)        launcher/InstanceImportTask.cpp
        └─► ModrinthCreationTask / FlameCreationTask (InstanceCreationTask)
              createInstance(): write instance.cfg + mmc-pack.json, download mods, copy overrides
              (this fork) → ServerUtils::PutManifest()
  └─► InstanceList commits the staging folder → instance appears
```

Updating a managed pack re-runs the same creation task with `updateInstance()` / `shouldOverride()`, deleting the
previous version's files (`m_files_to_remove`).

## API keys

CurseForge needs an API key (`APPLICATION->getFlameAPIKey()`, from `buildconfig/` or the Services settings page).
Requests built with `Net::ApiDownload` add it automatically through `ApiHeaderProxy`.
