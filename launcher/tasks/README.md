# `tasks/` — asynchronous work

Almost everything that takes time in the launcher is a `Task`: downloads (`NetJob`), instance creation
(`InstanceTask`s), launching (`LaunchTask`), Java detection, mod parsing...

| File | Role |
|---|---|
| `Task.h` | Base class. Lifecycle, status/progress/details signals, abort. **Read its class comment first.** |
| `ConcurrentTask.h` | Runs child tasks in parallel (up to N at a time); aggregates progress; base of `NetJob`. |
| `SequentialTask.h` | Runs child tasks one after another, stops at the first failure. |
| `MultipleOptionsTask.h` | Tries children in order until one succeeds (fallbacks). |

## Lifecycle

```
Inactive --start()--> Running --emitSucceeded()--> Succeeded --.
                         |----emitFailed(reason)--> Failed -----+--> finished()
                         '----abort()/emitAborted()--> AbortedByUser --'
```

- Subclasses implement `executeTask()`, which **must return quickly**: start async work (e.g. a `NetJob`) and call
  `emitSucceeded()` / `emitFailed(tr("why"))` later, from a slot.
- Callers connect to `succeeded`, `failed(QString)`, `finished`, then call `start()`.
- `setStatus()`, `setDetails()`, `setProgress(current, total)` feed progress dialogs.
- `setAbortable(true)` + override `abort()` if the task can be cancelled.

## Running a task

With a progress window (blocks the caller while the dialog is open, UI stays responsive):

```cpp
auto task = makeShared<MyTask>(...);
ProgressDialog dialog(this);
dialog.execWithTask(task.get());
if (task->wasSuccessful()) { ... }
```

Without UI:

```cpp
m_task = makeShared<MyTask>(...);          // keep a reference (member) until it finishes
connect(m_task.get(), &Task::succeeded, this, [this] { ... });
connect(m_task.get(), &Task::failed, this, [](QString reason) { qWarning() << reason; });
m_task->start();
```

Combining tasks:

```cpp
auto seq = makeShared<SequentialTask>(tr("Install things"));
seq->addTask(downloadTask);
seq->addTask(extractTask);
seq->start();
```

## Pitfalls

- `makeShared<T>()` (from `QObjectPtr.h`) creates a `shared_qobject_ptr`; if the last reference goes away the task is
  deleted mid-flight. Store it in a member.
- Don't call `emitSucceeded()` twice, and always end with exactly one of succeeded/failed/aborted, otherwise the
  progress dialog waits forever.
- Tasks are not threads: they run on the thread that started them. Heavy CPU work should go through
  `QtConcurrent::run` (see `InstanceImportTask` / mod parsing tasks for examples).
