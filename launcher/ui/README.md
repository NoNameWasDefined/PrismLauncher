# `ui/` — windows, dialogs, pages and widgets

Qt Widgets UI. Layouts are designed in Qt Designer `.ui` files (XML) that sit next to the `.cpp/.h` of the same
name; CMake turns `Foo.ui` into `ui_Foo.h` and the class uses it through `ui->someWidget`. Open `.ui` files with
Qt Designer / Qt Creator, or edit the XML by hand for small changes.

## Main pieces

| File / folder | Role |
|---|---|
| `MainWindow.h/.cpp/.ui` | Main window: toolbar actions (Add instance, Launch, Edit, Settings...), instance grid, status bar, news. Most user flows start in its `on_action..._triggered()` slots. |
| `InstanceWindow.h` | Per-instance window (log console while running + the instance pages). |
| `instanceview/` | The grouped icon grid of instances (custom view, delegate, proxy model). |
| `dialogs/` | Modal dialogs: `NewInstanceDialog`, `ProgressDialog`, `CopyInstanceDialog`, `ExportInstanceDialog`, `ResourceDownloadDialog`, `MSALoginDialog`... |
| `pages/` | Everything shown in a "page container" — see below. |
| `widgets/` | Reusable widgets (`PageContainer`, `LogView`, `VersionSelectWidget`, `JavaSettingsWidget`, `WideBar`...). |
| `themes/` | Themes, icon themes and the background cat. |
| `setupwizard/` | First-run wizard. |
| `java/` | Java download dialog. |
| `GuiUtil.h` | Log upload, clipboard and file dialog helpers. |

## The page system

A **page** is a `QWidget` that also implements `BasePage` (`pages/BasePage.h`): `id()`, `displayName()`, `icon()`,
`apply()`, `openedImpl()/closedImpl()`. A **page provider** lists pages; a **page container** shows the list on the
left and the selected page on the right.

| Where it appears | Pages folder | Registered in |
|---|---|---|
| Settings window | `pages/global/` | `Application.cpp` → `m_globalSettingsProvider->addPage<LauncherPage>()` ... |
| Instance window / "Edit instance" | `pages/instance/` | `InstancePageProvider.h` → `values.append(new ModFolderPage(...))` ... |
| "Add Instance" dialog | `pages/modplatform/` (CustomPage, ImportPage, ModrinthPage, FlamePage...) | `dialogs/NewInstanceDialog.cpp` → `pages.append(...)` |
| Download mods/packs dialog | `pages/modplatform/*Resource*` + `modrinth/`, `flame/` | `dialogs/ResourceDownloadDialog.cpp` |

New-instance pages create the task to run: they call `dialog->setSuggestedPack(name, task)` with an `InstanceTask`
(e.g. `new InstanceImportTask(url, ...)`), and `NewInstanceDialog` returns it to `MainWindow`, which runs it in
`MainWindow::instanceFromInstanceTask()` through `InstanceList::wrapInstanceTask()` (creates the staging folder, then
moves the result into `instances/` on success).

## Recipes

**Add a field to an existing settings page** (e.g. a sync server URL in *Settings → Launcher*):

1. Add a widget (e.g. `QLineEdit` named `syncServerUrlEdit`) to `pages/global/LauncherPage.ui`.
2. In `LauncherPage::loadSettings()`:
   `ui->syncServerUrlEdit->setText(s->get("ModpackSyncServerURL").toString());`
3. In `LauncherPage::applySettings()`:
   `s->set("ModpackSyncServerURL", ui->syncServerUrlEdit->text());`
4. The setting must already be registered (it is, in `Application.cpp`).

**Add a new settings page:** copy a small page (e.g. `pages/global/ProxyPage.*`), rename the class and its `id()`,
add the three files to `launcher/CMakeLists.txt` (`.cpp/.h` in `LAUNCHER_SOURCES`, `.ui` in `qt_wrap_ui(LAUNCHER_UI ...)`),
then `m_globalSettingsProvider->addPage<MyPage>();` in `Application.cpp`.

**Add an instance page:** same idea in `pages/instance/`, then `values.append(new MyPage(onesix.get()));` in
`InstancePageProvider.h`. `shouldDisplay()` can hide it for some instances.

**Run a task with a progress window:** `ProgressDialog dlg(this); dlg.execWithTask(task.get());`

**Show a message:** `CustomMessageBox::selectable(this, tr("Title"), tr("Text"), QMessageBox::Warning)->show();`
(or a plain `QMessageBox`). Write UI strings in English inside `tr()`.
