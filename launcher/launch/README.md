# `launch/` — the launch pipeline

Launching an instance = running a `LaunchTask`, an ordered list of `LaunchStep`s.

```
MainWindow "Launch" button
  └─► Application::launch(instance)                ../Application.cpp   (this fork: modpack sync check here)
        └─► LaunchController                        ../LaunchController.cpp
              • choose account / refresh Microsoft login / offline name   → AuthSession
              • instance->createLaunchTask(session, target)               ../minecraft/MinecraftInstance.cpp
              └─► LaunchTask::start()  runs the steps one by one:
                    TextPrint ─ CreateGameFolders ─ [LookupServerAddress] ─ MinecraftLoadAndCheck
                    ─ AutoInstallJava ─ CheckJava ─ [PreLaunchCommand]
                    ─ (online) ClaimAccount + update tasks (libraries, assets, ...)
                    ─ ModMinecraftJar ─ ScanModFolders ─ PrintInstanceInfo ─ ExtractNatives
                    ─ ReconstructAssets ─ VerifyJavaInstall ─ LauncherPartLaunch (java process)
                    ─ [PostLaunchCommand] ─ [QuitAfterGameStop]
```

`[...]` = only added when configured. Generic steps live in `steps/`; Minecraft-specific ones in
[`../minecraft/launch`](../minecraft/launch).

| File | Role |
|---|---|
| `LaunchTask.h` | The step runner; owns the `LogModel`; censors secrets in logs. Big comment on top. |
| `LaunchStep.h` | Base class for steps, with a minimal example. |
| `TaskStepWrapper.h` | Run any `Task` as a step. |
| `LogModel.h` | Log lines shown in the instance's log page. |
| `steps/*` | Generic steps (Java check, pre/post commands, DNS lookup, quit after stop...). |

## Details worth knowing

- **Order matters**: a step that fails stops the launch, and `finalize()` is then called on every step that already
  ran, in reverse order.
- **Pausing**: a step can emit `readyForLaunch()` to pause the chain (used for profilers). `LaunchController` resumes it
  with `LaunchTask::proceed()`.
- **The game process** is started by `LauncherPartLaunch`: it runs Java with the instance's JVM args and the
  `NewLaunch.jar` from [`../../libraries/launcher`](../../libraries/launcher), then writes the launch script
  (`MinecraftInstance::createLaunchScript()`) to its stdin. The step finishes when the game exits.
- **Logs**: use `emit logLine(text, MessageLevel::Launcher)` inside a step. Game output goes through
  `LogParser` (Log4j XML or plain text) and `censorPrivateInfo()`.

## Add a step

1. Create `MyStep.h/.cpp` deriving from `LaunchStep` (see the example in `LaunchStep.h`), add them to
   `LAUNCH_SOURCES` or `MINECRAFT_SOURCES` in `launcher/CMakeLists.txt`.
2. In `MinecraftInstance::createLaunchTask()`, `process->appendStep(makeShared<MyStep>(pptr));` at the right place
   (e.g. right before the `LauncherPartLaunch` block to run just before the game starts).

A modpack sync implemented as a step would show its progress in the launch log, run after the account is ready, and
could block the launch cleanly with `emitFailed(...)` — an alternative to the blocking call in `Application::launch()`.
