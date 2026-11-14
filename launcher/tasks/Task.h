// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PrismLauncher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (c) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

// File overview: Base class for all asynchronous work (downloads, installs, launches...): status text, progress, abort,
// succeeded/failed/finished signals. See launcher/tasks/README.md.

#pragma once

#include <QLoggingCategory>
#include <QRunnable>
#include <QUuid>

#include "QObjectPtr.h"

Q_DECLARE_LOGGING_CATEGORY(taskLogC)

// State of one sub-step, as shown in the ProgressDialog's list of sub-task progress bars.
enum class TaskStepState { Waiting, Running, Failed, Succeeded };

Q_DECLARE_METATYPE(TaskStepState)

// Progress report of one sub-step of a bigger task (e.g. one file of a NetJob).
// Tasks made of sub-tasks (ConcurrentTask, NetJob...) emit stepProgress() with one of these per child,
// identified by `uid`, so the UI can draw one progress bar per running child.
struct TaskStepProgress {
    QUuid uid;
    qint64 current = 0;
    qint64 total = -1;

    qint64 old_current = 0;
    qint64 old_total = -1;

    QString status = "";
    QString details = "";
    TaskStepState state = TaskStepState::Waiting;

    TaskStepProgress() { this->uid = QUuid::createUuid(); }
    TaskStepProgress(QUuid uid_) : uid(uid_) {}

    bool isDone() const { return (state == TaskStepState::Failed) || (state == TaskStepState::Succeeded); }
    void update(qint64 new_current, qint64 new_total)
    {
        this->old_current = this->current;
        this->old_total = this->total;

        this->current = new_current;
        this->total = new_total;
        this->state = TaskStepState::Running;
    }
};

Q_DECLARE_METATYPE(TaskStepProgress)

using TaskStepProgressList = QList<std::shared_ptr<TaskStepProgress>>;

/*!
 * Represents a task that has to be done.
 * To create a task, you need to subclass this class, implement the executeTask() method and call
 * emitSucceeded() or emitFailed() when the task is done.
 * the caller needs to call start() to start the task.
 *
 * Lifecycle: Inactive --start()--> Running --emitSucceeded()/emitFailed()/emitAborted()--> Succeeded/Failed/AbortedByUser.
 * Every terminal transition also emits finished(), so connect to finished() for cleanup and to
 * succeeded()/failed() for the outcome.
 *
 * Typical subclass:
 *
 *     class MyTask : public Task {
 *         Q_OBJECT
 *       protected:
 *         void executeTask() override {
 *             setStatus(tr("Doing the thing"));        // text shown in progress dialogs
 *             // start async work here (e.g. a NetJob) and return immediately;
 *             // when it ends call emitSucceeded() or emitFailed(tr("why"))
 *         }
 *     };
 *
 * Typical use from UI code:
 *
 *     auto task = makeShared<MyTask>();
 *     ProgressDialog dialog(this);
 *     dialog.execWithTask(task.get());   // starts the task and blocks with a progress bar
 *
 * or without UI: connect to the signals, then call task->start().
 * Tasks run on the thread that started them; executeTask() must not block the event loop.
 */
class Task : public QObject, public QRunnable {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<Task>;

    enum class State { Inactive, Running, Succeeded, Failed, AbortedByUser };

   public:
    explicit Task(bool show_debug_log = true);
    virtual ~Task() = default;

    bool isRunning() const;
    bool isFinished() const;
    bool wasSuccessful() const;

    /*!
     * MultiStep tasks are combinations of multiple tasks into a single logical task.
     * The main usage of this is in SequencialTask.
     */
    virtual auto isMultiStep() const -> bool { return false; }

    /*!
     * Returns the string that was passed to emitFailed as the error message when the task failed.
     * If the task hasn't failed, returns an empty string.
     */
    QString failReason() const;

    virtual QStringList warnings() const;

    virtual bool canAbort() const { return m_can_abort; }

    auto getState() const -> State { return m_state; }

    QString getStatus() { return m_status; }
    QString getDetails() { return m_details; }

    qint64 getProgress() { return m_progress; }
    qint64 getTotalProgress() { return m_progressTotal; }
    virtual auto getStepProgress() const -> TaskStepProgressList { return {}; }

    QUuid getUid() { return m_uid; }

   protected:
    void logWarning(const QString& line);

   private:
    QString describe();

   signals:
    //! emitted by start(), right before executeTask() runs
    void started();
    //! overall progress; total <= 0 means "unknown" (busy indicator)
    void progress(qint64 current, qint64 total);
    //! called when a task has either succeeded, aborted or failed.
    void finished();
    //! called when a task has succeeded
    void succeeded();
    //! called when a task has been aborted by calling abort()
    void aborted();
    //! called when a task has failed; `reason` is user-facing text (also available later via failReason())
    void failed(QString reason);
    //! main status line shown in progress dialogs (set via setStatus())
    void status(QString status);
    //! secondary, more detailed line (e.g. current file name), set via setDetails()
    void details(QString details);
    //! a non-fatal warning was recorded with logWarning(); collected in warnings()
    void warningLogged(const QString& warning);
    //! progress of one sub-step (see TaskStepProgress); forwarded upwards by parent tasks
    void stepProgress(TaskStepProgress const& task_progress);

    //! Emitted when the canAbort() status has changed. */
    void abortStatusChanged(bool can_abort);

   public slots:
    // QRunnable's interface
    void run() override { start(); }

    //! used by the task caller to start the task
    virtual void start();
    //! used by external code to ask the task to abort
    virtual bool abort()
    {
        if (canAbort())
            emitAborted();
        return canAbort();
    }

    void setAbortable(bool can_abort)
    {
        m_can_abort = can_abort;
        emit abortStatusChanged(can_abort);
    }

   protected:
    //! The task subclass must implement this method. This method is called to start to run the task.
    //! The task is not finished when this method returns. the subclass must manually call emitSucceeded() or emitFailed() instead.
    virtual void executeTask() = 0;

   protected slots:
    //! The Task subclass must call this method when the task has succeeded
    virtual void emitSucceeded();
    //! **The Task subclass** must call this method when the task has aborted. External code should call abort() instead.
    virtual void emitAborted();
    //! The Task subclass must call this method when the task has failed
    virtual void emitFailed(QString reason = "");

    virtual void propagateStepProgress(TaskStepProgress const& task_progress);

   public slots:
    void setStatus(const QString& status);
    void setDetails(const QString& details);
    void setProgress(qint64 current, qint64 total);

   protected:
    State m_state = State::Inactive;
    QStringList m_Warnings;
    QString m_failReason = "";
    QString m_status;
    QString m_details;
    int m_progress = 0;
    int m_progressTotal = 100;

    // TODO: Nuke in favor of QLoggingCategory
    bool m_show_debug = true;

   private:
    // Change using setAbortStatus
    bool m_can_abort = false;
    QUuid m_uid;
};
