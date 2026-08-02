#include "profiler.h"

#include <string.h>

bool profilerEnabled = true;

static ProfileStats profileStats[PROFILE_COUNT];

void profilerReset() {
  memset(profileStats, 0, sizeof(profileStats));
}

uint32_t profilerBegin() {
  if (!profilerEnabled) return 0;
  return micros();
}

void profilerEnd(ProfileSection section, uint32_t startUs) {
  if (!profilerEnabled || startUs == 0 || section >= PROFILE_COUNT) return;
  uint32_t elapsedUs = (uint32_t)(micros() - startUs);
  ProfileStats &s = profileStats[section];
  s.calls++;
  s.totalUs += elapsedUs;
  if (elapsedUs > s.maxUs) s.maxUs = elapsedUs;
  if (elapsedUs >= 50000UL) s.slow50ms++;
  if (elapsedUs >= 100000UL) s.slow100ms++;
  if (elapsedUs >= 500000UL) s.slow500ms++;
  if (elapsedUs >= 1000000UL) s.slow1000ms++;
}

const char* profilerSectionName(ProfileSection section) {
  switch (section) {
    case PROFILE_LOOP: return "loop";
    case PROFILE_MOUNT_POLL: return "mount poll";
    case PROFILE_HTTP_SERVERS: return "http servers";
    case PROFILE_ALPACA_DISCOVERY: return "alpaca discovery";
    case PROFILE_LX200_SERVER: return "lx200 server";
    case PROFILE_LX200_COMMAND: return "lx200 command";
    case PROFILE_LX200_WAIT_SERVICE: return "lx200 wait service";
    case PROFILE_STELLARIUM_SERVER: return "stellarium server";
    case PROFILE_TELNET_CONSOLE: return "telnet console";
    case PROFILE_NTP_SYNC: return "ntp sync";
    case PROFILE_GOTO_COMPLETION: return "goto completion";
    case PROFILE_GOTO_QUEUE: return "goto queue";
    case PROFILE_ASYNC_SLEW: return "async slew";
    case PROFILE_CONSOLE: return "console";
    case PROFILE_RESTART: return "restart";
    default: return "unknown";
  }
}

void profilerPrint(Print &out) {
  out.printf("Profiler: %s\n", profilerEnabled ? "enabled" : "disabled");
  out.println("Section                 Calls      Max ms   Avg us   >50ms  >100ms >500ms >1000ms");
  for (uint8_t i = 0; i < PROFILE_COUNT; i++) {
    const ProfileStats &s = profileStats[i];
    uint32_t avgUs = s.calls ? (uint32_t)(s.totalUs / s.calls) : 0;
    out.printf("%-22s %9lu %8.3f %8lu %6lu %6lu %6lu %7lu\n",
               profilerSectionName((ProfileSection)i),
               (unsigned long)s.calls,
               (double)s.maxUs / 1000.0,
               (unsigned long)avgUs,
               (unsigned long)s.slow50ms,
               (unsigned long)s.slow100ms,
               (unsigned long)s.slow500ms,
               (unsigned long)s.slow1000ms);
  }
}

String profilerText() {
  String s;
  s.reserve(1600);
  s += "=== PROFILER ===\n";
  s += "State: ";
  s += profilerEnabled ? "enabled\n" : "disabled\n";
  s += "Section                 Calls      Max ms   Avg us   >50ms  >100ms >500ms >1000ms\n";
  char line[128];
  for (uint8_t i = 0; i < PROFILE_COUNT; i++) {
    const ProfileStats &st = profileStats[i];
    uint32_t avgUs = st.calls ? (uint32_t)(st.totalUs / st.calls) : 0;
    snprintf(line, sizeof(line), "%-22s %9lu %8.3f %8lu %6lu %6lu %6lu %7lu\n",
             profilerSectionName((ProfileSection)i),
             (unsigned long)st.calls,
             (double)st.maxUs / 1000.0,
             (unsigned long)avgUs,
             (unsigned long)st.slow50ms,
             (unsigned long)st.slow100ms,
             (unsigned long)st.slow500ms,
             (unsigned long)st.slow1000ms);
    s += line;
  }
  return s;
}

String profilerJson() {
  String s;
  s.reserve(1800);
  s += "\"profilerEnabled\":";
  s += profilerEnabled ? "true" : "false";
  s += ",\"profile\":[";
  for (uint8_t i = 0; i < PROFILE_COUNT; i++) {
    if (i) s += ",";
    const ProfileStats &st = profileStats[i];
    uint32_t avgUs = st.calls ? (uint32_t)(st.totalUs / st.calls) : 0;
    s += "{\"name\":\"";
    s += profilerSectionName((ProfileSection)i);
    s += "\",\"calls\":";
    s += String(st.calls);
    s += ",\"maxUs\":";
    s += String(st.maxUs);
    s += ",\"avgUs\":";
    s += String(avgUs);
    s += ",\"slow50ms\":";
    s += String(st.slow50ms);
    s += ",\"slow100ms\":";
    s += String(st.slow100ms);
    s += ",\"slow500ms\":";
    s += String(st.slow500ms);
    s += ",\"slow1000ms\":";
    s += String(st.slow1000ms);
    s += "}";
  }
  s += "]";
  return s;
}
