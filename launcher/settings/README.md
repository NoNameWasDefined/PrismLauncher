# `settings/` — settings storage

| File | Role |
|---|---|
| `SettingsObject.h` | A set of named settings: `registerSetting`, `get`, `set`, `reset`. |
| `INISettingsObject.h` | A `SettingsObject` saved to an INI file (`prismlauncher.cfg`, `instance.cfg`). |
| `Setting.h` | One setting: id, default value, old names ("synonyms"), change signal. |
| `OverrideSetting.h` / `PassthroughSetting.h` | Instance settings that fall back to the global value unless an "override" checkbox (gate setting) is on. |
| `INIFile.h` | The INI reader/writer. |

Two `SettingsObject`s matter:

- **Global**: `APPLICATION->settings()` → `prismlauncher.cfg`. Every key is registered in
  `Application::Application()` in [`../Application.cpp`](../Application.cpp) (search for `registerSetting`).
- **Per instance**: `instance->settings()` → `instances/<id>/instance.cfg`. Registered in
  `BaseInstance`'s constructor and `MinecraftInstance::loadSpecificSettings()`.

## Add a setting

1. Register it with its default value:
   ```cpp
   m_settings->registerSetting("MyOption", false);              // global, in Application.cpp
   ```
2. Use it:
   ```cpp
   bool on = APPLICATION->settings()->get("MyOption").toBool();
   APPLICATION->settings()->set("MyOption", true);              // saved automatically
   ```
3. Show it in the UI: add a widget to the page's `.ui` file (e.g. `ui/pages/global/LauncherPage.ui`), load the value in
   the page's `loadSettings()` and write it in `applySettings()`. See [`../ui/README.md`](../ui/README.md).

`get()` on a key that was never registered silently returns an invalid `QVariant` (so `.toBool()` is `false`,
`.toString()` is empty), and `set()` fails with an error in the log. A typo in a key name therefore shows up as
"the setting is always empty" — register first and copy-paste the key.

## Instance setting that overrides a global one

```cpp
// in MinecraftInstance::loadSpecificSettings(), like the existing OverrideJavaLocation/JavaPath pair
if (auto global_settings = globalSettings()) {
    auto gate = m_settings->registerSetting("OverrideMyOption", false);
    m_settings->registerOverride(global_settings->getSetting("MyOption"), gate);
}
```

Then `instance->settings()->get("MyOption")` returns the instance value when the gate is on, else the global one.
