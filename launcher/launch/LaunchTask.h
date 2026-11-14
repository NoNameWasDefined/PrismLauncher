// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
 *      Authors: Orochimarufan <orochimarufan.x3@gmail.com>
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

// File overview: Runs the ordered chain of LaunchSteps that starts an instance, collects the log into a LogModel and
// manages the game process lifetime. See launcher/launch/README.md.

#pragma once
#include <QObjectPtr.h>
#include <minecraft/MinecraftInstance.h>
#include <QProcess>
#include "BaseInstance.h"
#include "LaunchStep.h"
#include "LogModel.h"
#include "MessageLevel.h"
#include "logs/LogParser.h"

/*
 * How a launch works:
 *
 *   LaunchController (UI side: account selection, offline name...)
 *     -> MinecraftInstance::createLaunchTask(session, target) builds a LaunchTask and appends steps:
 *          TextPrint (folder info) -> CreateGameFolders -> [LookupServerAddress]
 *          -> MinecraftLoadAndCheck (reload mmc-pack.json + meta)  -> AutoInstallJava -> CheckJava
 *          -> [PreLaunchCommand] -> (online only) ClaimAccount + update tasks (libraries, assets...)
 *          -> ModMinecraftJar -> ScanModFolders -> PrintInstanceInfo -> ExtractNatives
 *          -> ReconstructAssets -> VerifyJavaInstall -> LauncherPartLaunch (starts Java)
 *          -> [PostLaunchCommand] -> [QuitAfterGameStop]
 *     -> start(): steps run one after the other (onStepFinished() starts the next one).
 *
 * A step may pause the chain by emitting readyForLaunch() (e.g. LauncherPartLaunch waiting for a
 * profiler to attach); proceed() resumes it. When the chain ends (success, failure or abort),
 * finalize() is called on every step that ran, in reverse order, so steps can undo things
 * (release the account, delete the temporary modded jar...).
 *
 * Every step's log lines go through onLogLine(s) -> censorPrivateInfo() -> m_logModel, which the
 * instance window's log page displays.
 *
 * To add your own step: subclass LaunchStep, then appendStep()/insert it in
 * MinecraftInstance::createLaunchTask at the right place.
 */
class LaunchTask : public Task {
    Q_OBJECT
   protected:
    explicit LaunchTask(MinecraftInstancePtr instance);
    void init();

   public:
    // Waiting = paused by a step (readyForLaunch), resumed with proceed().
    enum State { NotStarted, Running, Waiting, Failed, Aborted, Finished };

   public: /* methods */
    static shared_qobject_ptr<LaunchTask> create(MinecraftInstancePtr inst);
    virtual ~LaunchTask() = default;

    // Steps must be added before start(); they run in list order.
    void appendStep(shared_qobject_ptr<LaunchStep> step);
    void prependStep(shared_qobject_ptr<LaunchStep> step);
    // Map of secret -> replacement (access token, uuid...) applied to every log line before it is shown/uploaded.
    void setCensorFilter(QMap<QString, QString> filter);

    MinecraftInstancePtr instance() { return m_instance; }

    void setPid(qint64 pid) { m_pid = pid; }

    qint64 pid() { return m_pid; }

    /**
     * @brief prepare the process for launch (for multi-stage launch)
     */
    virtual void executeTask() override;

    /**
     * @brief launch the armed instance
     */
    void proceed();

    /**
     * @brief abort launch
     */
    bool abort() override;

    bool canAbort() const override;

    shared_qobject_ptr<LogModel> getLogModel();

   public:
    // Replaces $INST_NAME, $INST_ID, $INST_DIR, $INST_MC_DIR, $INST_JAVA, $INST_JAVA_ARGS... in user commands
    // (pre-launch, wrapper, post-exit). The variables come from MinecraftInstance::getVariables().
    QString substituteVariables(QString& cmd, bool isLaunch = false) const;
    // Applies the censor filter (see setCensorFilter).
    QString censorPrivateInfo(QString in);

   protected: /* methods */
    virtual void emitFailed(QString reason) override;
    virtual void emitSucceeded() override;

   signals:
    /**
     * @brief emitted when the launch preparations are done
     */
    void readyForLaunch();

    void requestProgress(Task* task);

    void requestLogging();

   public slots:
    void onLogLines(const QStringList& lines, MessageLevel defaultLevel = MessageLevel::Launcher);
    void onLogLine(QString line, MessageLevel defaultLevel = MessageLevel::Launcher);
    void onReadyForLaunch();
    void onStepFinished();
    void onProgressReportingRequested();

   private: /*methods */
    void finalizeSteps(bool successful, const QString& error);

   protected:
    bool parseXmlLogs(QString const& line, MessageLevel level);

   protected: /* data */
    MinecraftInstancePtr m_instance;
    shared_qobject_ptr<LogModel> m_logModel;
    QList<shared_qobject_ptr<LaunchStep>> m_steps;
    QMap<QString, QString> m_censorFilter;
    int currentStep = -1;
    State state = NotStarted;
    qint64 m_pid = -1;
    LogParser m_stdoutParser;
    LogParser m_stderrParser;
};
