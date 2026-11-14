/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// File overview: Base class for one step of a LaunchTask (check Java, run pre-launch command, start game...). Steps can
// log lines, pause the launch and clean up in finalize().

#pragma once

#include "MessageLevel.h"
#include "tasks/Task.h"

#include <QStringList>

class LaunchTask;
/*
 * One step of a launch. Implement executeTask() like any Task and finish with emitSucceeded()/emitFailed();
 * a failed step stops the whole launch. Use emit logLine(...) to write to the instance's log window,
 * and m_parent->instance() to reach the instance being launched.
 *
 * Minimal example:
 *
 *     class SayHello : public LaunchStep {
 *         Q_OBJECT
 *       public:
 *         explicit SayHello(LaunchTask* parent) : LaunchStep(parent) {}
 *         void executeTask() override {
 *             emit logLine("Hello from " + m_parent->instance()->name(), MessageLevel::Launcher);
 *             emitSucceeded();
 *         }
 *         bool canAbort() const override { return false; }
 *     };
 */
class LaunchStep : public Task {
    Q_OBJECT
   public: /* methods */
    explicit LaunchStep(LaunchTask* parent);
    virtual ~LaunchStep() = default;

   signals:
    // Lines sent to the LaunchTask's log (shown in the instance log page, coloured by level).
    void logLines(QStringList lines, MessageLevel level);
    void logLine(QString line, MessageLevel level);
    // Pause the launch chain until LaunchTask::proceed() is called (see proceed()).
    void readyForLaunch();
    void progressReportingRequest();

   public slots:
    virtual void proceed() {};
    // called in the opposite order than the Task launch(), used to clean up or otherwise undo things after the launch ends
    virtual void finalize() {};

   protected: /* data */
    LaunchTask* m_parent;
};
