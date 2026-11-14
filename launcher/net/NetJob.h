// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
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

// File overview: A ConcurrentTask that runs several NetRequests in parallel (downloads/uploads) with retry support. The
// usual way to download things. See launcher/net/README.md.

#pragma once

#include <QtNetwork>

#include <QObject>
#include "net/NetRequest.h"
#include "tasks/ConcurrentTask.h"

// Those are included so that they are also included by anyone using NetJob
#include "net/Download.h"
#include "net/HttpMetaCache.h"

/*
 * The standard way to do HTTP in the launcher (asynchronous, with progress, retry and abort).
 *
 *     auto output = std::make_shared<QByteArray>();
 *     auto job = makeShared<NetJob>(tr("Fetch something"), APPLICATION->network());
 *     job->addNetAction(Net::ApiDownload::makeByteArray(QUrl("https://example.org/data.json"), output));
 *     // or Net::ApiDownload::makeFile(url, "/path/to/file")
 *     // or Net::ApiDownload::makeCached(url, APPLICATION->metacache()->resolveEntry("general", "relative/path"))
 *     connect(job.get(), &NetJob::succeeded, this, [output] { auto doc = QJsonDocument::fromJson(*output); ... });
 *     connect(job.get(), &NetJob::failed, this, [](QString reason) { qWarning() << reason; });
 *     job->start();   // keep `job` alive (e.g. store it in a member) until it finishes
 *
 * Use Net::ApiDownload/ApiUpload (adds API keys + user agent) for platform APIs, Net::Download/Upload otherwise.
 * Several actions added to one job run in parallel (max_concurrent, default from settings). When some fail,
 * the user may be asked whether to retry (setAskRetry(false) disables that).
 *
 * Prefer this over blocking QEventLoop code in the UI thread: the UI stays responsive and the task can be
 * shown in a ProgressDialog.
 */
class NetJob : public ConcurrentTask {
    Q_OBJECT

   public:
    using Ptr = shared_qobject_ptr<NetJob>;

    explicit NetJob(QString job_name, shared_qobject_ptr<QNetworkAccessManager> network, int max_concurrent = -1);
    ~NetJob() override = default;

    auto size() const -> int;

    auto canAbort() const -> bool override;
    auto addNetAction(Net::NetRequest::Ptr action) -> bool;

    auto getFailedActions() -> QList<Net::NetRequest*>;
    auto getFailedFiles() -> QList<QString>;
    void setAskRetry(bool askRetry);

   public slots:
    // Qt can't handle auto at the start for some reason?
    bool abort() override;
    void emitFailed(QString reason) override;

   protected slots:
    void executeNextSubTask() override;

   protected:
    void updateState() override;
    bool isOnline();

   private:
    shared_qobject_ptr<QNetworkAccessManager> m_network;

    int m_try = 1;
    bool m_ask_retry = true;
    int m_manual_try = 0;
};
