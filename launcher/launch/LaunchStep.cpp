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

// File overview: Implementation of LaunchStep.h. Base class for one step of a LaunchTask (check Java, run pre-launch
// command, start game...). Steps can log lines, pause the launch and clean up in finalize().

#include "LaunchStep.h"
#include "LaunchTask.h"

LaunchStep::LaunchStep(LaunchTask* parent) : Task(), m_parent(parent)
{
    connect(this, &LaunchStep::readyForLaunch, parent, &LaunchTask::onReadyForLaunch);
    connect(this, &LaunchStep::logLine, parent, &LaunchTask::onLogLine);
    connect(this, &LaunchStep::logLines, parent, &LaunchTask::onLogLines);
    connect(this, &LaunchStep::finished, parent, &LaunchTask::onStepFinished);
    connect(this, &LaunchStep::progressReportingRequest, parent, &LaunchTask::onProgressReportingRequested);
}
