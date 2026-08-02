#include "crashdump.h"

#if defined(ESP32)
  #include "esp_heap_caps.h"
  #include "esp_system.h"
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
#endif

static const uint32_t CRASHDUMP_MAGIC = 0xBC506258UL;
static const uint8_t CRASHDUMP_COUNT = 40;
static const uint8_t CRASHDUMP_SERVICE_COUNT = 28;

struct CrashDumpEntry {
  uint32_t ms;
  uint32_t heap;
  uint32_t largest;
  uint8_t core;
  char event[28];
  char detail[32];
};

struct CrashDumpService {
  uint32_t ms;
  uint32_t hits;
  char name[24];
};

struct CrashDumpStore {
  uint32_t magic;
  uint32_t bootCount;
  uint32_t next;
  uint32_t total;
  uint32_t heartbeatMs;
  uint32_t heartbeatCount;
  uint32_t heartbeatHeap;
  uint32_t heartbeatLargest;
  uint32_t heartbeatCore;
  char heartbeatDetail[32];
  char currentSection[32];
  CrashDumpService services[CRASHDUMP_SERVICE_COUNT];
  CrashDumpEntry entries[CRASHDUMP_COUNT];
};

#if defined(ESP32)
RTC_NOINIT_ATTR static CrashDumpStore crashdumpStore;
#else
static CrashDumpStore crashdumpStore;
#endif

static void copyText(char *dst, size_t dstSize, const char *src) {
  if (!dst || dstSize == 0) return;
  if (!src) src = "";
  size_t i = 0;
  for (; i + 1 < dstSize && src[i]; i++) dst[i] = src[i];
  dst[i] = '\0';
}

static bool crashdumpValid() {
  return crashdumpStore.magic == CRASHDUMP_MAGIC &&
         crashdumpStore.next < CRASHDUMP_COUNT &&
         crashdumpStore.total < 1000000UL;
}

static void crashdumpResetStore() {
  memset(&crashdumpStore, 0, sizeof(crashdumpStore));
  crashdumpStore.magic = CRASHDUMP_MAGIC;
}

static const char *crashdumpResetReasonText() {
#if defined(ESP32)
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON: return "power-on";
    case ESP_RST_EXT: return "external reset";
    case ESP_RST_SW: return "software restart";
    case ESP_RST_PANIC: return "panic/exception";
    case ESP_RST_INT_WDT: return "interrupt watchdog";
    case ESP_RST_TASK_WDT: return "task watchdog";
    case ESP_RST_WDT: return "other watchdog";
    case ESP_RST_DEEPSLEEP: return "deep sleep wake";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_SDIO: return "SDIO reset";
    default: return "unknown";
  }
#else
  return "unknown";
#endif
}

static const CrashDumpEntry &crashdumpAt(uint32_t orderedIndex) {
  uint32_t count = crashdumpStore.total < CRASHDUMP_COUNT ? crashdumpStore.total : CRASHDUMP_COUNT;
  uint32_t start = crashdumpStore.total < CRASHDUMP_COUNT ? 0 : crashdumpStore.next;
  return crashdumpStore.entries[(start + orderedIndex) % CRASHDUMP_COUNT];
}

void crashdumpEvent(const char *event, const char *detail) {
  if (!crashdumpValid()) {
    crashdumpResetStore();
  }

  CrashDumpEntry &e = crashdumpStore.entries[crashdumpStore.next];
  e.ms = millis();
  e.heap = ESP.getFreeHeap();
#if defined(ESP32)
  e.largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  e.core = (uint8_t)xPortGetCoreID();
#else
  e.largest = e.heap;
  e.core = 0;
#endif
  copyText(e.event, sizeof(e.event), event);
  copyText(e.detail, sizeof(e.detail), detail);
  if (strcmp(event, "begin") == 0) {
    copyText(crashdumpStore.currentSection, sizeof(crashdumpStore.currentSection), detail);
  } else if (strcmp(event, "end") == 0 && detail && strcmp(crashdumpStore.currentSection, detail) == 0) {
    copyText(crashdumpStore.currentSection, sizeof(crashdumpStore.currentSection), "idle");
  }

  crashdumpStore.next = (crashdumpStore.next + 1) % CRASHDUMP_COUNT;
  if (crashdumpStore.total < 1000000UL) crashdumpStore.total++;
}

void crashdumpHeartbeat(const char *detail) {
  if (!crashdumpValid()) crashdumpResetStore();
  crashdumpStore.heartbeatMs = millis();
  crashdumpStore.heartbeatCount++;
  crashdumpStore.heartbeatHeap = ESP.getFreeHeap();
#if defined(ESP32)
  crashdumpStore.heartbeatLargest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  crashdumpStore.heartbeatCore = (uint32_t)xPortGetCoreID();
#else
  crashdumpStore.heartbeatLargest = crashdumpStore.heartbeatHeap;
  crashdumpStore.heartbeatCore = 0;
#endif
  copyText(crashdumpStore.heartbeatDetail, sizeof(crashdumpStore.heartbeatDetail), detail);
  crashdumpEvent("heartbeat", detail);
}

void crashdumpMarkService(const char *service) {
  if (!service || !service[0]) return;
  if (!crashdumpValid()) crashdumpResetStore();
  int freeSlot = -1;
  for (uint8_t i = 0; i < CRASHDUMP_SERVICE_COUNT; i++) {
    if (crashdumpStore.services[i].name[0] == '\0') {
      if (freeSlot < 0) freeSlot = i;
      continue;
    }
    if (strncmp(crashdumpStore.services[i].name, service, sizeof(crashdumpStore.services[i].name)) == 0) {
      crashdumpStore.services[i].ms = millis();
      crashdumpStore.services[i].hits++;
      return;
    }
  }
  if (freeSlot >= 0) {
    CrashDumpService &s = crashdumpStore.services[freeSlot];
    s.ms = millis();
    s.hits = 1;
    copyText(s.name, sizeof(s.name), service);
  }
}

void crashdumpPrintLive(Print &out) {
  if (!crashdumpValid()) {
    out.println("No crashdump live state recorded.");
    return;
  }
  unsigned long now = millis();
  out.println("=== Crashdump Live State ===");
  out.printf("Reset reason: %s\n", crashdumpResetReasonText());
  out.printf("Uptime ms: %lu\n", now);
  out.printf("Heartbeat count: %lu\n", (unsigned long)crashdumpStore.heartbeatCount);
  out.printf("Last heartbeat age: %lu ms\n",
             crashdumpStore.heartbeatMs ? now - crashdumpStore.heartbeatMs : 0UL);
  out.printf("Last heartbeat: %lu ms core %lu heap=%lu largest=%lu detail=%s\n",
             (unsigned long)crashdumpStore.heartbeatMs,
             (unsigned long)crashdumpStore.heartbeatCore,
             (unsigned long)crashdumpStore.heartbeatHeap,
             (unsigned long)crashdumpStore.heartbeatLargest,
             crashdumpStore.heartbeatDetail);
  out.printf("Current section: %s\n", crashdumpStore.currentSection[0] ? crashdumpStore.currentSection : "unknown");
  out.println();
  out.println("service                  hits       age ms     last ms");
  for (uint8_t i = 0; i < CRASHDUMP_SERVICE_COUNT; i++) {
    const CrashDumpService &s = crashdumpStore.services[i];
    if (!s.name[0]) continue;
    out.printf("%-24s %-10lu %-10lu %lu\n",
               s.name,
               (unsigned long)s.hits,
               s.ms ? now - s.ms : 0UL,
               (unsigned long)s.ms);
  }
  out.println("=== End Crashdump Live State ===");
}

void crashdumpPrint(Print &out) {
  if (!crashdumpValid() || crashdumpStore.total == 0) {
    out.println("No previous crashdump recorded.");
    return;
  }

  uint32_t count = crashdumpStore.total < CRASHDUMP_COUNT ? crashdumpStore.total : CRASHDUMP_COUNT;
  const CrashDumpEntry &newest = crashdumpAt(count - 1);
  out.println("=== Last Firmware Crash Dump ===");
  out.printf("Reset reason: %s\n", crashdumpResetReasonText());
  out.printf("Boot count seen by crashdump: %lu\n", (unsigned long)crashdumpStore.bootCount);
  out.printf("Stored entries: %lu shown of %lu total writes\n",
             (unsigned long)count,
             (unsigned long)crashdumpStore.total);
  out.printf("Newest entry: %lu ms core %u heap=%lu largest=%lu %s %s\n",
             (unsigned long)newest.ms,
             (unsigned)newest.core,
             (unsigned long)newest.heap,
             (unsigned long)newest.largest,
             newest.event,
             newest.detail);
  if (strcmp(newest.event, "begin") == 0) {
    out.print("Interpretation: newest entry is a section begin; if this was printed after a crash/reset, suspect this section: ");
    out.println(newest.detail);
  } else if (strcmp(newest.event, "end") == 0) {
    out.print("Interpretation: newest entry is a section end; the last completed section was: ");
    out.println(newest.detail);
  } else {
    out.println("Interpretation: newest entry is a boot marker or one-shot event.");
  }
  out.println("Note: entries are held in RTC memory; they usually survive watchdog/panic/software resets, not full power loss.");
  out.println();
  out.println("ms        core heap     largest  event                       detail");
  for (uint32_t i = 0; i < count; i++) {
    const CrashDumpEntry &e = crashdumpAt(i);
    out.printf("%-9lu %-4u %-8lu %-8lu %-27s %s\n",
               (unsigned long)e.ms,
               (unsigned)e.core,
               (unsigned long)e.heap,
               (unsigned long)e.largest,
               e.event,
               e.detail);
  }
  out.println("=== End Crash Dump ===");
}

void crashdumpBegin() {
  bool hadPrevious = crashdumpValid() && crashdumpStore.total > 0;
  if (!crashdumpValid()) {
    crashdumpResetStore();
  }

  crashdumpStore.bootCount++;
  if (hadPrevious) {
    crashdumpPrint(Serial);
  } else {
    Serial.println("No previous crashdump recorded.");
  }
  crashdumpEvent("boot", "setup begin");
}

CrashDumpScope::CrashDumpScope(const char *name) : _name(name) {
  crashdumpEvent("begin", _name);
}

CrashDumpScope::~CrashDumpScope() {
  crashdumpEvent("end", _name);
}
