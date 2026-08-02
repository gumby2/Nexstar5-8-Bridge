#pragma once

#define NEXSTAR_DIAGNOSTIC_LIGHT 1

#include <Arduino.h>

String formatUptime();
String resetReasonText();
String basicSystemHealthText();
String systemHealthText();
String taskStatsSectionText(bool basicMode);
String sampleWebCpuLoadText();
const char *sampleBannerSystemText();
String taskRefreshText();
String monitorRefreshText();
void printTaskRuntimeStats(Print &out);
void telnetDrawMonitor(Print &out);
void telnetDrawTasks(Print &out);
