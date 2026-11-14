// File overview: Basic system information (OS name, CPU architecture, total RAM) used for logs and defaults.

#pragma once
#include <QString>

namespace SysInfo {
QString currentSystem();
QString useQTForArch();
QString getSupportedJavaArchitecture();
int suitableMaxMem();
}  // namespace SysInfo
