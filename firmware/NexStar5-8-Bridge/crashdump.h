#pragma once

#include <Arduino.h>

void crashdumpBegin();
void crashdumpEvent(const char *event, const char *detail = nullptr);
void crashdumpPrint(Print &out);
void crashdumpHeartbeat(const char *detail = nullptr);
void crashdumpMarkService(const char *service);
void crashdumpPrintLive(Print &out);

class CrashDumpScope {
public:
  explicit CrashDumpScope(const char *name);
  ~CrashDumpScope();

private:
  const char *_name;
};

#define CRASHDUMP_SCOPE_CONCAT_INNER(a, b) a##b
#define CRASHDUMP_SCOPE_CONCAT(a, b) CRASHDUMP_SCOPE_CONCAT_INNER(a, b)
#define CRASHDUMP_SCOPE(name) CrashDumpScope CRASHDUMP_SCOPE_CONCAT(crashdumpScopeInstance_, __LINE__)(name)
