#pragma once

#include <Arduino.h>
#include <stdint.h>

enum ProfileSection : uint8_t {
  PROFILE_LOOP = 0,
  PROFILE_MOUNT_POLL,
  PROFILE_HTTP_SERVERS,
  PROFILE_ALPACA_DISCOVERY,
  PROFILE_LX200_SERVER,
  PROFILE_LX200_COMMAND,
  PROFILE_LX200_WAIT_SERVICE,
  PROFILE_STELLARIUM_SERVER,
  PROFILE_TELNET_CONSOLE,
  PROFILE_NTP_SYNC,
  PROFILE_GOTO_COMPLETION,
  PROFILE_GOTO_QUEUE,
  PROFILE_ASYNC_SLEW,
  PROFILE_CONSOLE,
  PROFILE_RESTART,
  PROFILE_COUNT
};

struct ProfileStats {
  uint32_t calls;
  uint64_t totalUs;
  uint32_t maxUs;
  uint32_t slow50ms;
  uint32_t slow100ms;
  uint32_t slow500ms;
  uint32_t slow1000ms;
};

extern bool profilerEnabled;

void profilerReset();
uint32_t profilerBegin();
void profilerEnd(ProfileSection section, uint32_t startUs);
const char* profilerSectionName(ProfileSection section);
String profilerText();
void profilerPrint(Print &out);
String profilerJson();

class ProfileScope {
public:
  explicit ProfileScope(ProfileSection section) : section_(section), startUs_(profilerBegin()) {}
  ~ProfileScope() { profilerEnd(section_, startUs_); }
private:
  ProfileSection section_;
  uint32_t startUs_;
};

#define PROFILE_SCOPE_CONCAT_INNER(a, b) a##b
#define PROFILE_SCOPE_CONCAT(a, b) PROFILE_SCOPE_CONCAT_INNER(a, b)
#define PROFILE_SCOPE(section) ProfileScope PROFILE_SCOPE_CONCAT(profileScopeInstance_, __LINE__)(section)
