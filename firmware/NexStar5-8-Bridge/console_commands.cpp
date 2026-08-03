#include "crashdump.h"
#include "console_commands.h"

#include "bluetooth_services.h"
#include "logging.h"
#include "mount_transport.h"
#include "network_services.h"
#include "nexstar_protocol.h"
#include "position_cache.h"
#include "profiler.h"
#include "settings.h"
#include "slew_controller.h"
#include "slew_web_handlers.h"
#include "time_services.h"
#include <math.h>
#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#else
  #include <WiFi.h>
  #include "esp_heap_caps.h"
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
#endif

extern const char* FW_VERSION;
extern const char* FW_NAME;
extern String serialPendingConfirmCommand;
extern String serialPendingConfirmDescription;
extern unsigned long lastLoopTimeMs;
extern unsigned long maxLoopTimeMs;
extern unsigned long lastLoopLatencyMs;
extern unsigned long maxLoopLatencyMs;

static bool consoleParseTwoValues(const String &text, double &first, double &second) {
  String args = text;
  args.trim();
  int split = args.indexOf(' ');
  if (split <= 0) return false;
  String firstText = args.substring(0, split);
  String secondText = args.substring(split + 1);
  firstText.trim();
  secondText.trim();
  if (!firstText.length() || !secondText.length() || secondText.indexOf(' ') >= 0) return false;
  first = firstText.toFloat();
  second = secondText.toFloat();
  return isfinite(first) && isfinite(second);
}

static void consoleQueueGotoRaDec(const String &args) {
  double raHours = 0.0, decDeg = 0.0;
  if (!consoleParseTwoValues(args, raHours, decDeg) || raHours < 0.0 || raHours >= 24.0 || decDeg < -90.0 || decDeg > 90.0) {
    Serial.println("Usage: goto radec <RA hours 0..24) <Dec degrees -90..90>");
    return;
  }
  String safetyReason;
  double raDeg = normalizeRA(raHours * 15.0);
  if (!allowSlewRaDecBySafety(raDeg, decDeg, safetyReason)) {
    Serial.print("GOTO blocked: ");
    Serial.println(safetyReason);
    return;
  }
  if (slewControllerPendingOrRunning()) {
    Serial.println("GOTO rejected: another mount command is already queued or running.");
    return;
  }
  markPositionDemand("Serial RA/Dec GOTO");
  queueAsyncSlew(raDeg, decDeg);
  Serial.printf("GOTO RA/Dec queued: %.6f hours %.6f degrees\n", raHours, decDeg);
}

static void consoleQueueGotoAltAz(const String &args) {
  double altDeg = 0.0, azDeg = 0.0;
  if (!consoleParseTwoValues(args, altDeg, azDeg) || altDeg < -90.0 || altDeg > 90.0 || azDeg < 0.0 || azDeg >= 360.0) {
    Serial.println("Usage: goto altaz <altitude degrees -90..90> <azimuth degrees 0..360)");
    return;
  }
  String safetyReason;
  altDeg = clampAlt(altDeg);
  azDeg = normalizeAz(azDeg);
  if (!allowSlewAltAzBySafety(altDeg, azDeg, safetyReason)) {
    Serial.print("GOTO blocked: ");
    Serial.println(safetyReason);
    return;
  }
  if (slewControllerPendingOrRunning()) {
    Serial.println("GOTO rejected: another mount command is already queued or running.");
    return;
  }
  markPositionDemand("Serial Alt/Az GOTO");
  queueAsyncAltAzSlew(altDeg, azDeg);
  Serial.printf("GOTO Alt/Az queued: %.6f degrees %.6f degrees\n", altDeg, azDeg);
}

extern uint16_t logCategoryBitFromName(const String &name);
extern String currentStateText();
extern String formatUptime();
extern String resetReasonText();
extern String runtimeApSsid();
extern String telnetMaskedPassword();
extern void printTelnetRuntimeStatus(Print &out);
extern void printTaskRuntimeStats(Print &out);
extern const char* lx200SourceName(uint8_t source);

String consoleLogMaskNames(uint16_t mask) {
  if (mask == 0) return "none";
  if ((mask & LOG_CAT_ALL) == LOG_CAT_ALL) return "all";

  String s = "";
  if (mask & LOG_CAT_MOUNT)      s += "mount,";
  if (mask & LOG_CAT_SKYSAFARI)  s += "skysafari,";
  if (mask & LOG_CAT_BLUETOOTH)  s += "bluetooth,";
  if (mask & LOG_CAT_ALPACA)     s += "alpaca,";
  if (mask & LOG_CAT_STELLARIUM) s += "stellarium,";
  if (mask & LOG_CAT_WEB)        s += "web,";
  if (mask & LOG_CAT_WIFI)       s += "wifi,";
  if (mask & LOG_CAT_TIMELOC)    s += "time_location,";
  if (mask & LOG_CAT_SETTINGS)   s += "settings,";
  if (mask & LOG_CAT_SYSTEM)     s += "system,";
  if (s.endsWith(",")) s.remove(s.length() - 1);
  return s.length() ? s : "none";
}

void printConsoleLogStatus() {
  Serial.println("Log status:");
  Serial.printf("  level: %d (%s)\n", LOG_LEVEL, logLevelName(LOG_LEVEL));
  Serial.printf("  system mask: 0x%04X\n", LOG_SUBSYSTEM_MASK);
  Serial.print("  systems: ");
  Serial.println(consoleLogMaskNames(LOG_SUBSYSTEM_MASK));
}


uint16_t consoleLogMaskFromList(String cats) {
  cats.trim();
  cats.toLowerCase();
  cats.replace(",", " ");
  cats.replace(";", " ");
  cats.replace("|", " ");
  cats.replace("+", " ");

  if (cats == "all") return LOG_CAT_ALL;
  if (cats == "none" || cats == "off" || cats.length() == 0) return 0;

  uint16_t mask = 0;
  int start = 0;
  while (start < (int)cats.length()) {
    while (start < (int)cats.length() && cats.charAt(start) == ' ') start++;
    if (start >= (int)cats.length()) break;

    int sp = cats.indexOf(' ', start);
    String part = sp >= 0 ? cats.substring(start, sp) : cats.substring(start);
    part.trim();

    if (part == "bt") part = "bluetooth";
    else if (part == "sky") part = "skysafari";
    else if (part == "safari") part = "skysafari";
    else if (part == "time") part = "time_location";
    else if (part == "timeloc") part = "time_location";
    else if (part == "location") part = "time_location";
    else if (part == "config") part = "settings";
    else if (part == "setup") part = "settings";
    else if (part == "wifi") part = "wifi";
    else if (part == "wi-fi") part = "wifi";

    uint16_t bit = logCategoryBitFromName(part);
    if (bit == 0 && part.length() > 0) {
      Serial.print("[console] unknown log system ignored: ");
      Serial.println(part);
    }
    mask |= bit;

    if (sp < 0) break;
    start = sp + 1;
  }
  return mask;
}

void printHelpLog() {
  Serial.println();
  Serial.println("=== Logging Commands ===");
  Serial.println("  log");
  Serial.println("      Show the current log level, subsystem mask, and enabled subsystems.");
  Serial.println("  logging | logging 0 | logging 1");
  Serial.println("      Show, disable, or enable persistent firmware logging.");
  Serial.println("  log <0..5>");
  Serial.println("      Set log level:");
  Serial.println("        0 none   Disable stored/streamed firmware log output.");
  Serial.println("        1 error  Only failures that usually need action.");
  Serial.println("        2 warn   Errors plus abnormal/retry/fallback conditions.");
  Serial.println("        3 info   Normal high-level events: clients, saves, connect/disconnect.");
  Serial.println("        4 debug  Detailed decisions and state changes for troubleshooting.");
  Serial.println("        5 trace  Very chatty low-level flow; use briefly, can flood logs.");
  Serial.println("  log <0..5> [systems]");
  Serial.println("      Set log level and one or more subsystems separated by spaces or commas.");
  Serial.println("      Systems: all, none, mount, skysafari, bluetooth, wifi, web, alpaca,");
  Serial.println("               stellarium, time_location, settings, system");
  Serial.println("      Aliases: bt=bluetooth, sky/lx200=skysafari, time/location/ntp=time_location,");
  Serial.println("               config/setup/littlefs=settings, sys=system");
  Serial.println("      Examples: log 3 system   |   log 4 mount bluetooth   |   log 5 mount,skysafari");
  Serial.println("      SkySafari position-poll traffic is intentionally suppressed to prevent log flooding.");
}

void printHelpMode() {
  Serial.println();
  Serial.println("=== Mode and Runtime Commands ===");
  Serial.println("  mode");
  Serial.println("      Show current mode and active services.");
  Serial.println("  mode bt");
  Serial.println("      Save Bluetooth + WiFi Telnet mode, then reboot.");
  Serial.println("  mode wifi");
  Serial.println("      Save Full WiFi web/protocol mode, then reboot.");
  Serial.println("  mode wifi noweb");
  Serial.println("      Save WiFi protocol servers with browser Web UI disabled, then reboot.");
  Serial.println("  mode web");
  Serial.println("      Save Web UI + Telnet only mode, then reboot.");
  Serial.println("  web | web status | webserver | webserver status");
  Serial.println("      Show Web UI state. Use mode commands to change saved Web UI behavior.");
  Serial.println("  wifi | wifi status");
  Serial.println("      Show WiFi runtime, AP/STA connection, and IP information.");
  Serial.println("  wifi on | wifi off");
  Serial.println("      Explicitly change WiFi runtime state. In BT mode, Bluetooth remains active when WiFi is off.");
  Serial.println("  reboot | restart");
  Serial.println("      Restart the controller immediately.");
}

void printHelpWifi() {
  Serial.println();
  Serial.println("=== WiFi Configuration Commands ===");
  Serial.println("  setsta <ssid> <password>");
  Serial.println("      Save STA credentials and immediately attempt a connection.");
  Serial.println("      SSIDs and passwords containing spaces are not supported by this console command.");
  Serial.println("  apdefault");
  Serial.println("      Restore AP SSID, password, and IP defaults, then restart the AP.");
  Serial.println("  wifi | wifi status");
  Serial.println("      Show current WiFi and IP status.");
  Serial.println("  wifi on | wifi off");
  Serial.println("      Enable or disable WiFi at runtime.");
  Serial.println("      Use the Full WiFi Setup page for complete STA and AP configuration.");
}

void printHelpMount() {
  Serial.println();
  Serial.println("=== Mount and Polling Commands ===");
  Serial.println("  testinit");
  Serial.println("  rawmount [cycles] [ez|z] [interval_ms]");
  Serial.println("  rawgoto [cycles] [wait|nowait] [interval_ms]");
  Serial.println("      rawmount tests handshake and fixed-length E/Z payload reads.");
  Serial.println("      rawgoto reads current RA/Dec, then sends it as an R GOTO.");
  Serial.println("      Use wait to consume '@' completion and drain trailing bytes.");
  Serial.println("      nowait is low-level and may leave delayed bytes for the next command.");
  Serial.println("  get");
  Serial.println("      Query and print current RA/Dec.");
  Serial.println("  getaltaz");
  Serial.println("      Query and print current Alt/Az.");
  Serial.println("  pos | current_state | currentstate | state");
  Serial.println("      Print cached telescope coordinates and current bridge state.");
  Serial.println("  rates");
  Serial.println("      Show all four nudge rates, mount poll interval, and client-poll throttle.");
  Serial.println("  mountpoll | mountpoll <ms>");
  Serial.println("      Get or set the mount poll interval. 0 turns it off.");
  Serial.println("  handshake | handshake <ms>");
  Serial.println("      Get or set NexStar handshake timeout. Range 500..10000 ms.");
  Serial.println("  nudge az+ | nudge az- | nudge alt+ | nudge alt-");
  Serial.println("      Perform one relative Alt/Az nudge using nudge-rate slot 2.");
  Serial.println("  drain");
  Serial.println("      Drain and print pending bytes from the mount serial input.");
}

void printHelpStatus() {
  Serial.println();
  Serial.println("=== Status and Diagnostics Commands ===");
  Serial.println("  status");
  Serial.println("      Show firmware, mode, network, clients, GOTO queue, heap, loop latency,");
  Serial.println("      poll scheduler timing, and mount communication status.");
  Serial.println("  current_state | currentstate | state | pos");
  Serial.println("      Show current cached telescope coordinates and operational state.");
  Serial.println("  system_health | systemhealth | health");
  Serial.println("      Show system-health counters, loop and poll latency, memory, uptime, and errors.");
  Serial.println("  profile | profile 0 | profile 1 | profile clear");
  Serial.println("      Show, disable/enable, or clear low-overhead service timing counters.");
  Serial.println("  crashdump");
  Serial.println("      Show the last firmware activities preserved across reset when possible.");
  Serial.println("  tasks");
  Serial.println("      Show FreeRTOS runtime task stats if enabled in the build.");
  Serial.println("  gpio_startup");
  Serial.println("      Show startup GPIO override state, selected pin, priority, and forced modes.");
  Serial.println("  telnet");
  Serial.println("      Show Telnet runtime status and counters.");
  Serial.println("  telnet down | telnet 0");
  Serial.println("      Stop/unload only the Telnet server; WiFi stays on.");
  Serial.println("  telnet on | telnet 1");
  Serial.println("      Start only the Telnet server; WiFi is unchanged.");
}

void printHelpTopic(const String &topicRaw) {
  String topic = topicRaw;
  topic.trim();
  topic.toLowerCase();

  if (topic == "mount" || topic == "scope" || topic == "nexstar") {
    printHelpMount();
  } else if (topic == "mode" || topic == "boot" || topic == "runtime" || topic == "radio") {
    printHelpMode();
  } else if (topic == "wifi" || topic == "network") {
    printHelpWifi();
  } else if (topic == "log" || topic == "logging" || topic == "logs") {
    printHelpLog();
  } else if (topic == "web" || topic == "webserver") {
    Serial.println();
    Serial.println("=== Web Server Command ===");
    Serial.println("  web | web status | webserver | webserver status");
    Serial.println("      Show Web UI state. Use mode commands to change saved Web UI behavior.");
  } else if (topic == "reboot" || topic == "restart") {
    Serial.println();
    Serial.println("=== Reboot Command ===");
    Serial.println("  reboot | restart");
    Serial.println("      Restart the controller immediately.");
  } else if (topic == "testinit") {
    Serial.println();
    Serial.println("=== testinit ===");
    Serial.println("  testinit");
    Serial.println("      Test the NexStar '?' handshake and safe E-command payload read.");
  } else if (topic == "get") {
    Serial.println();
    Serial.println("=== get ===");
    Serial.println("  get");
    Serial.println("      Query the mount and print current RA/Dec.");
  } else if (topic == "getaltaz") {
    Serial.println();
    Serial.println("=== getaltaz ===");
    Serial.println("  getaltaz");
    Serial.println("      Query the mount and print current Alt/Az.");
  } else if (topic == "rates" || topic == "poll" || topic == "mountpoll") {
    Serial.println();
    Serial.println("=== Mount Poll / Rates ===");
    Serial.println("  rates");
    Serial.println("      Show all four nudge rates, mount poll interval, and client-poll throttle.");
    Serial.println("  mountpoll | mountpoll <ms>");
    Serial.println("      Get or set the mount poll interval. 0 turns it off.");
    Serial.println("  handshake | handshake <ms>");
    Serial.println("      Get or set NexStar handshake timeout. Range 500..10000 ms.");
  } else if (topic == "nudge") {
    Serial.println();
    Serial.println("=== nudge ===");
    Serial.println("  nudge az+ | nudge az- | nudge alt+ | nudge alt-");
    Serial.println("      Perform one relative Alt/Az nudge using nudge-rate slot 2.");
  } else if (topic == "drain") {
    Serial.println();
    Serial.println("=== drain ===");
    Serial.println("  drain");
    Serial.println("      Drain and print pending bytes from the mount serial input.");
  } else if (topic == "pos" || topic == "state" || topic == "current_state" || topic == "currentstate") {
    Serial.println();
    Serial.println("=== Current State Command ===");
    Serial.println("  current_state | currentstate | state | pos");
    Serial.println("      Show cached telescope coordinates and current bridge state.");
  } else if (topic == "health" || topic == "system_health" || topic == "systemhealth") {
    Serial.println();
    Serial.println("=== System Health Command ===");
    Serial.println("  system_health | systemhealth | health");
    Serial.println("      Show counters, loop and poll latency, memory, uptime, and errors.");
  } else if (topic == "profile" || topic == "profiler") {
    Serial.println();
    Serial.println("=== profile ===");
    Serial.println("  profile");
    Serial.println("      Show service timing counters.");
    Serial.println("  profile 0 | profile 1");
    Serial.println("      Disable or enable profiling.");
    Serial.println("  profile clear");
    Serial.println("      Reset profile counters.");
  } else if (topic == "crashdump") {
    Serial.println();
    Serial.println("=== crashdump ===");
    Serial.println("  crashdump");
    Serial.println("      Show the last recorded firmware activities.");
    Serial.println("      Automatically printed at boot when previous entries survived reset.");
    Serial.println("      Useful after watchdog/crash resets to see what began or completed last.");
    Serial.println("  crashdump live");
    Serial.println("      Show current heartbeat and service liveness ages.");
    Serial.println("      Columns:");
    Serial.println("        ms      milliseconds since that boot");
    Serial.println("        core    CPU core that wrote the entry");
    Serial.println("        heap    free heap at the entry");
    Serial.println("        largest largest free heap block at the entry");
    Serial.println("        event   begin, end, or boot");
    Serial.println("        detail  firmware section name");
    Serial.println("      If the newest previous entry is 'begin <section>' with no matching end,");
    Serial.println("      the reset/freeze probably happened inside that section.");
  } else if (topic == "tasks") {
    Serial.println();
    Serial.println("=== tasks ===");
    Serial.println("  tasks");
    Serial.println("      Show FreeRTOS cumulative per-task runtime stats if enabled in this build.");
    Serial.println("      Serial is one-shot. Telnet task screen is disabled in this release.");
    Serial.println("      Fields: Task=name, Abs Time=cumulative CPU ticks, % Time=CPU share since boot.");
    Serial.println("      Telnet CPU Load is interval load from IDLE0/IDLE1 deltas; first sample shows --%.");
    Serial.println("      Common tasks:");
    Serial.println("        IDLE0/IDLE1 idle tasks for core 0/1; high values mean spare CPU.");
    Serial.println("        tiT TCP/IP stack; Tmr Svc FreeRTOS software timers.");
    Serial.println("        ipc0/ipc1 inter-core calls; esp_timer high-resolution timer callbacks.");
    Serial.println("        wifi WiFi driver; sys_evt ESP system events; arduino_events Arduino events.");
  } else if (topic == "gpio_startup") {
    Serial.println();
    Serial.println("=== Startup GPIO Command ===");
    Serial.println("  gpio_startup");
    Serial.println("      Show startup GPIO override state, selected pin, pin priority, and the mode each pin forces.");
  } else if (topic == "telnet") {
    Serial.println();
    Serial.println("=== Telnet Status Command ===");
    Serial.println("  telnet");
    Serial.println("      Show Telnet port, connection, authentication, and command counters.");
  } else if (topic == "telnetlog") {
    Serial.println();
    Serial.println("=== telnetlog ===");
    Serial.println("  telnetlog");
    Serial.println("      Show whether log lines are streamed to the Telnet terminal.");
    Serial.println("  telnetlog 0");
    Serial.println("      Turn off log streaming to the Telnet terminal.");
    Serial.println("  telnetlog 1");
    Serial.println("      Turn on log streaming to the Telnet terminal.");
  } else if (topic == "idlepoll") {
    Serial.println();
    Serial.println("=== idlepoll ===");
    Serial.println("  idlepoll | poll idle");
    Serial.println("      Show idle poll setting. Active clients use mountpoll; idle uses idlepoll.");
    Serial.println("  idlepoll <ms> | poll idle <ms>");
    Serial.println("      Save idle poll interval used when no active client is demanding position.");
  } else if (topic == "setsta" || topic == "sta") {
    Serial.println();
    Serial.println("=== setsta ===");
    Serial.println("  setsta <ssid> <password>");
    Serial.println("      Save STA credentials and immediately attempt a connection.");
    Serial.println("      Spaces in SSID or password are not supported by this command.");
  } else if (topic == "apdefault" || topic == "ap") {
    Serial.println();
    Serial.println("=== apdefault ===");
    Serial.println("  apdefault");
    Serial.println("      Restore AP SSID, password, and IP defaults, then restart the AP.");
  } else {
    Serial.print("Unknown help topic: ");
    Serial.println(topicRaw);
    Serial.println("Grouped topics: mount, mode, wifi");
    Serial.println("Specific topics: log, web, reboot, testinit, get, getaltaz, rates, mountpoll,");
    Serial.println("                 handshake, profile, crashdump, nudge, idlepoll, drain, pos, health, tasks, gpio_startup, telnet,");
    Serial.println("                 telnetlog, setsta, apdefault");
  }
}

void printHelp() {
  Serial.println();
  Serial.printf("%s %s commands\n", FW_NAME, FW_VERSION);
  Serial.println("Help:");
  Serial.println("  help | ? | help mount | help mode | help wifi | help <command>");
  Serial.println("Status:");
  Serial.println("  status");
  Serial.println("  current_state | currentstate | state | pos");
  Serial.println("  system_health | systemhealth | health");
  Serial.println("  profile | profile 0 | profile 1 | profile clear");
  Serial.println("  crashdump");
  Serial.println("  tasks");
  Serial.println("  gpio_startup");
  Serial.println("  telnet");
  Serial.println("  telnetlog");
  Serial.println("Mount:");
  Serial.println("  testinit | rawmount [cycles] [ez|z] [interval_ms] | rawgoto [cycles] [wait|nowait] [interval_ms]");
  Serial.println("  get | getaltaz | goto radec <hours> <degrees> | goto altaz <degrees> <degrees>");
  Serial.println("  rates | drain");
  Serial.println("  mountpoll | mountpoll <ms>");
  Serial.println("  handshake | handshake <ms>");
  Serial.println("  idlepoll | idlepoll <ms> | poll idle <ms>");
  Serial.println("  nudge az+ | nudge az- | nudge alt+ | nudge alt-");
  Serial.println("Mode:");
  Serial.println("  mode");
  Serial.println("  mode bt | mode wifi | mode wifi noweb | mode web");
  Serial.println("  web | web status");
  Serial.println("  wifi | wifi status | wifi on | wifi off");
  Serial.println("  reboot | restart");
  Serial.println("Config:");
  Serial.println("  setsta <ssid> <password>");
  Serial.println("  apdefault");
  Serial.println("  telnetlog 0 | telnetlog 1");
  Serial.println("  log | log <0..5> [systems]");
}


void printConsoleStatus() {
  Serial.println();
  Serial.printf("Firmware: %s %s\n", FW_NAME, FW_VERSION);
  Serial.printf("Bridge mode: %s\n", bridgeModeName());
  Serial.printf("Startup mode source: %s\n", startupModeSource.c_str());
  Serial.printf("Startup mode pin used: %d\n", startupModePinUsed);
  Serial.printf("WiFi mode: %s\n", wifiModeText.c_str());
  Serial.printf("WiFi radio active: %d\n", (bridgeModeHasWebUi() || bridgeModeHasWifiProtocols()) ? 1 : 0);
  Serial.printf("WiFi status: %s\n", lastWifiStatus.c_str());
  Serial.printf("AP SSID: %s\n", apSsid.c_str());
  Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("STA configured: %d\n", staConfigured ? 1 : 0);
  Serial.printf("STA SSID: %s\n", staSsid.c_str());
  Serial.printf("STA connected: %d\n", (staConnected && WiFi.status() == WL_CONNECTED) ? 1 : 0);
  Serial.printf("STA IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Last STA IP: %s\n", lastStaIp.c_str());
  Serial.printf("Mount poll interval: %lu ms%s\n", pollIntervalMs, pollIntervalMs == 0 ? " (off)" : "");
  Serial.printf("Idle poll interval: %lu ms%s\n", idlePollIntervalMs, idlePollIntervalMs == 0 ? " (off)" : "");
  Serial.printf("WiFi runtime: %s\n", wifiRuntimeEnabled ? "enabled" : "disabled");
  Serial.printf("Telnet console: port=%u password=%s client=%s auth=%s commands=%lu authFailures=%lu\n",
                TELNET_PORT,
                telnetMaskedPassword().c_str(),
                (telnetClient && telnetClient.connected()) ? "connected" : "not connected",
                telnetAuthenticated ? "yes" : "no",
                (unsigned long)telnetRxCommands,
                (unsigned long)telnetAuthFailures);
  Serial.printf("Telnet live logs: %s lines=%lu\n", telnetLiveLogEnabled ? "enabled" : "disabled", (unsigned long)telnetLiveLogLines);
  Serial.printf("GOTO queue: active=%s type=%s timeout=%lu ms effective=%lu ms accepted=%lu started=%lu timedOut=%lu replaced=%lu immediateAcks=%lu cachedPositionReplies=%lu\n",
                hasQueuedGoto() ? "yes" : "no",
                queuedGotoTypeName(queuedGotoType),
                gotoQueueTimeoutMs,
                effectiveGotoQueueTimeoutMs(),
                (unsigned long)gotoQueueAccepted,
                (unsigned long)gotoQueueStarted,
                (unsigned long)gotoQueueTimedOut,
                (unsigned long)gotoQueueReplaced,
                (unsigned long)gotoQueueImmediateAcks,
                (unsigned long)queuedGotoPositionCacheReplies);
  Serial.printf("LX200 UI GOTO: active=%s source=%s started=%lu completed=%lu stopRequests=%lu age=%lu ms\n",
                lx200GotoUiActive ? "yes" : "no",
                lx200SourceName(lx200GotoUiSource),
                (unsigned long)lx200GotoUiStartedCount,
                (unsigned long)lx200GotoUiCompletedCount,
                (unsigned long)lx200GotoUiStopRequests,
                lx200GotoUiActive ? (unsigned long)(millis() - lx200GotoUiStartedMs) : 0UL);
  Serial.printf("Mount poll interval=%lu ms legacyIdle=%lu ms lastPollAge=%lu ms schedulerLatency=%lu/%lu ms started=%lu deferredBusy=%lu missedDeadline=%lu\n",
                pollIntervalMs,
                idlePollIntervalMs,
                lastMountPollMs == 0 ? 0UL : (unsigned long)(millis() - lastMountPollMs),
                lastPollSchedulerLatencyMs,
                maxPollSchedulerLatencyMs,
                mountPollsStarted,
                mountPollsDeferredBusy,
                mountPollsMissedDeadline);
  Serial.printf("Loop latency current/max=%lu/%lu ms\n", lastLoopLatencyMs, maxLoopLatencyMs);
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
#if defined(ESP32)
  Serial.printf("Min free heap: %u bytes\n", espMinFreeHeapCompat());
#endif
#if HAS_CLASSIC_BT
  Serial.printf("Bluetooth enabled: %d\n", bluetoothRuntimeIsEnabled() ? 1 : 0);
  Serial.printf("Bluetooth client: %d\n", bluetoothClientConnected() ? 1 : 0);
#else
  Serial.println("Bluetooth enabled: 0");
#endif
  Serial.printf("Mount fault: %d busy: %d\n", mountCommFault ? 1 : 0, mountBusy ? 1 : 0);
  Serial.println();
}

void printConsoleModeStatus(Print &out) {
  out.printf("Mode: %s\n", bridgeModeName());
  out.println("Runtime services:");
#if HAS_CLASSIC_BT
  out.printf("  Bluetooth: %s\n", bluetoothRuntimeIsEnabled() ? "enabled" : "disabled");
#else
  out.println("  Bluetooth: disabled");
#endif
  out.printf("  Telnet: %s %u\n", telnetConsoleServerRunning() ? "listening" : "disabled", TELNET_PORT);
  out.printf("  Web UI: %s %u\n", bridgeModeHasWebUi() ? "listening" : "disabled", bridgeModeHasWebUi() ? HTTP_WEB_PORT : 0);
  out.printf("  SkySafari/LX200: %s %u\n", bridgeModeHasWifiProtocols() ? (lx200WifiClientConnected ? "connected" : "idle") : "disabled", bridgeModeHasWifiProtocols() ? LX200_PORT : 0);
  out.printf("  Alpaca: %s %u\n", bridgeModeHasWifiProtocols() ? "listening" : "disabled", bridgeModeHasWifiProtocols() ? ALPACA_PORT : 0);
  out.printf("  Stellarium: %s %u\n", bridgeModeHasWifiProtocols() ? (stellariumClientConnected ? "connected" : "idle") : "disabled", bridgeModeHasWifiProtocols() ? STELLARIUM_PORT : 0);
}

bool consoleConfirmYes(const String &cmd) {
  return cmd == "y" || cmd == "yes";
}

bool consoleConfirmNo(const String &cmd) {
  return cmd.length() == 0 || cmd == "n" || cmd == "no";
}

bool commandRequiresDisconnectConfirm(const String &cmd, bool telnetSession, String &description) {
  if (cmd == "reboot" || cmd == "restart") {
    description = "This command will reboot the controller.";
    return true;
  }
  if (cmd == "wifi off") {
    description = "This command will turn WiFi off and may disconnect clients.";
    return true;
  }
  if (cmd == "mode bt" || cmd == "mode wifi" || cmd == "mode wifi noweb" || cmd == "mode web") {
    description = "This command will save boot mode and reboot the controller.";
    return true;
  }
  if (cmd == "apdefault") {
    description = "This command will restart the AP/radio path and may disconnect clients.";
    return true;
  }
  if (cmd.startsWith("setsta ")) {
    description = "This command will change STA WiFi and may disconnect clients.";
    return true;
  }
  if (telnetSession && (cmd == "exit" || cmd == "quit")) {
    description = "This command will close the Telnet session.";
    return true;
  }
  return false;
}

void printCommandConfirmPrompt(Print &out, const String &line, const String &description) {
  (void)line;
  out.println(description);
  out.print(" Proceed? [y/N]: ");
}

void printMountPollValue(Print &out) {
  out.printf("mountpoll=%lu ms%s\n", pollIntervalMs, pollIntervalMs == 0 ? " (off)" : "");
}

bool setMountPollValue(long valueMs) {
  if (valueMs < 0) valueMs = 0;
  if (valueMs > 60000) valueMs = 60000;
  pollIntervalMs = (unsigned long)valueMs;
  resetMountPollScheduler();
  return savePersistentSettings();
}

String formatAgeSeconds(unsigned long timestampMs) {
  if (timestampMs == 0) return "never";
  unsigned long s = (millis() - timestampMs) / 1000UL;
  if (s < 60UL) return String(s) + "s";
  unsigned long m = s / 60UL;
  s %= 60UL;
  if (m < 60UL) return String(m) + "m" + twoDigits(s) + "s";
  unsigned long h = m / 60UL;
  m %= 60UL;
  return String(h) + "h" + twoDigits(m) + "m";
}

void printConsoleHealth(Print &out) {
  out.println("=== System Health ===");
  out.println("POLL SCHEDULER");
  out.printf("Mount poll interval: %lu ms%s\n", pollIntervalMs, pollIntervalMs == 0 ? " (off)" : "");
  out.printf("Idle poll interval: %lu ms%s\n", idlePollIntervalMs, idlePollIntervalMs == 0 ? " (off)" : "");
  out.printf("Handshake timeout: %lu ms\n", mountHandshakeTimeoutMs);
  out.printf("Poll latency current/max: %lu/%lu ms\n",
             lastPollSchedulerLatencyMs,
             maxPollSchedulerLatencyMs);
  out.printf("Missed deadlines: %lu\n", mountPollsMissedDeadline);
  out.printf("Poll failures: %lu auto-disabled=%s\n",
             backgroundPollFailCount,
             backgroundPollingAutoDisabled ? "yes" : "no");
  out.printf("Mount uptime: %s h:mm\n", formatMountUptime().c_str());
  out.println();
  out.println("LOOP");
  out.printf("Loop latency current/max: %lu/%lu ms\n", lastLoopLatencyMs, maxLoopLatencyMs);
  out.printf("Loop execution current/max: %lu/%lu ms\n", lastLoopTimeMs, maxLoopTimeMs);
  out.printf("Uptime: %s\n", formatUptime().c_str());
  out.println();
  out.println("MEMORY");
  out.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
#if defined(ESP32)
  out.printf("Minimum free heap: %u bytes\n", espMinFreeHeapCompat());
  out.printf("Largest free block: %u bytes\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
  out.printf("Heap integrity: %s\n", heap_caps_check_integrity_all(false) ? "ok" : "FAILED");
  out.printf("Loop-task stack high-water: %u words\n", (unsigned)uxTaskGetStackHighWaterMark(NULL));
#endif
  out.printf("Reset reason: %s\n", resetReasonText().c_str());
  out.println();
  profilerPrint(out);
  out.println();
  out.println("NETWORK");
  out.printf("WiFi/bridge mode: %s / %s\n", wifiModeText.c_str(), bridgeModeName());
  out.printf("STA connected: %s\n", (staConnected && WiFi.status() == WL_CONNECTED) ? "yes" : "no");
  out.printf("AP clients: %d\n", WiFi.softAPgetStationNum());
  out.println("=== End System Health ===");
}

void handleConsole() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();
  String cmd = line;
  cmd.toLowerCase();

  Serial.print("[console] command echo: ");
  Serial.println(line);

  if (serialPendingConfirmCommand.length()) {
    if (consoleConfirmYes(cmd)) {
      line = serialPendingConfirmCommand;
      cmd = line;
      cmd.toLowerCase();
      serialPendingConfirmCommand = "";
      serialPendingConfirmDescription = "";
      Serial.println("Confirmed.");
    } else if (consoleConfirmNo(cmd)) {
      serialPendingConfirmCommand = "";
      serialPendingConfirmDescription = "";
      Serial.println("Cancelled.");
      return;
    } else {
      Serial.println("Please answer y or n. Blank cancels.");
      printCommandConfirmPrompt(Serial, serialPendingConfirmCommand, serialPendingConfirmDescription);
      return;
    }
  } else {
    String description;
    if (commandRequiresDisconnectConfirm(cmd, false, description)) {
      serialPendingConfirmCommand = line;
      serialPendingConfirmDescription = description;
      printCommandConfirmPrompt(Serial, line, description);
      return;
    }
  }

  if (cmd == "help" || cmd == "?") printHelp();
  else if (cmd.startsWith("help ")) printHelpTopic(line.substring(5));
  else if (cmd == "status") printConsoleStatus();

  else if (cmd == "gpio_startup") {
    Serial.print(gpioStartupModeText());
  }

  else if (cmd == "current_state" || cmd == "currentstate" || cmd == "state") {
    Serial.println();
    Serial.println("=== Current State ===");
    Serial.print(currentStateText());
    Serial.println("=== End Current State ===");
  }

  else if (cmd == "system_health" || cmd == "systemhealth" || cmd == "health") {
    Serial.println();
    printConsoleHealth(Serial);
  }

  else if (cmd == "profile" || cmd == "profiler") {
    profilerPrint(Serial);
  }

  else if (cmd == "crashdump live") {
    crashdumpPrintLive(Serial);
  }

  else if (cmd == "crashdump") {
    crashdumpPrint(Serial);
  }

  else if (cmd == "profile clear" || cmd == "profiler clear") {
    profilerReset();
    Serial.println("Profiler counters cleared.");
  }

  else if (cmd == "profile 0" || cmd == "profiler 0" || cmd == "profile off" || cmd == "profiler off") {
    profilerEnabled = false;
    Serial.println("Profiler disabled.");
  }

  else if (cmd == "profile 1" || cmd == "profiler 1" || cmd == "profile on" || cmd == "profiler on") {
    profilerEnabled = true;
    Serial.println("Profiler enabled.");
  }

  else if (cmd == "tasks") {
    printTaskRuntimeStats(Serial);
  }

  else if (cmd == "reboot" || cmd == "restart") {
    if (bridgeMode == BRIDGE_MODE_BT_MIN_WEB)
      Serial.println("BT mode will restart with Telnet only; no web interface.");
    Serial.println("Reboot command received; restarting ESP32...");
    delay(150);
    ESP.restart();
  }

  else if (cmd == "web" || cmd == "web status" || cmd == "webserver" || cmd == "webserver status") {
    Serial.printf("Web UI: %s\n", bridgeModeHasWebUi() ? "listening" : "disabled");
    Serial.println("Use mode wifi, mode wifi noweb, mode web, or mode bt to change saved Web UI behavior.");
  }

  else if (cmd == "wifi" || cmd == "wifi status") {
    Serial.printf("WiFi runtime: %s\n", wifiRuntimeEnabled ? "enabled" : "disabled");
    Serial.printf("Web interface: %s\n",
                  bridgeModeHasWebUi() ? "enabled by mode" : "disabled by mode");
    Serial.printf("WiFi mode text: %s\n", wifiModeText.c_str());
    Serial.printf("AP running: %d AP IP: %s STA connected: %d STA IP: %s\n",
                  apRunning ? 1 : 0,
                  WiFi.softAPIP().toString().c_str(),
                  (staConnected && WiFi.status() == WL_CONNECTED) ? 1 : 0,
                  WiFi.localIP().toString().c_str());
    Serial.println("Commands: wifi on | wifi off");
  }

  else if (cmd == "wifi off") {
    LOG_WIFI_I("Explicit WiFi runtime off accepted: serial");
    runtimeWifiOff();
  }

  else if (cmd == "wifi on") {
    LOG_WIFI_I("Explicit WiFi runtime on accepted: serial");
    runtimeWifiOn();
  }

  else if (cmd == "telnet" || cmd == "telnet status") {
    printTelnetRuntimeStatus(Serial);
  }

  else if (cmd == "telnet down" || cmd == "telnet off" || cmd == "telnet 0") {
    Serial.println("Stopping and unloading only the Telnet server; WiFi remains on...");
    LOG_WIFI_I("Explicit Telnet server down accepted: serial");
    stopTelnetConsoleServer("serial telnet command");
  }

  else if (cmd == "telnet on" || cmd == "telnet 1") {
    LOG_WIFI_I("Explicit Telnet server on accepted: serial");
    if (!wifiRuntimeEnabled) {
      Serial.println("Telnet cannot start because WiFi is off. Use 'wifi on' first.");
    } else if (!telnetConsoleServerRunning()) {
      startTelnetConsoleServer("serial telnet command");
    }
    Serial.println(telnetConsoleServerRunning() ? "Telnet server is ON; WiFi was unchanged." : "Telnet start failed.");
    printTelnetRuntimeStatus(Serial);
  }

  else if (cmd == "mode") {
    printConsoleModeStatus(Serial);
  }

  else if (cmd == "mode bt") {
    saveBluetoothLiteWebBootEnabled(false);
    saveBridgeMode(BRIDGE_MODE_BT_MIN_WEB);
    scheduleRestart("serial mode bt");
    Serial.println("Saved BT + WiFi Telnet mode; restarting. Web UI and WiFi protocol servers will not start.");
  }

  else if (cmd == "mode wifi") {
    saveBridgeMode(BRIDGE_MODE_WIFI_FULL);
    scheduleRestart("serial mode wifi");
    Serial.println("Saved full WiFi web mode; restarting...");
  }

  else if (cmd == "mode wifi noweb") {
    saveBridgeMode(BRIDGE_MODE_WIFI_SERVERS);
    scheduleRestart("serial mode wifi noweb");
    Serial.println("Saved WiFi protocol-server mode with browser Web UI disabled; restarting...");
  }

  else if (cmd == "mode web") {
    saveBridgeMode(BRIDGE_MODE_WEB_ONLY);
    scheduleRestart("serial mode web");
    Serial.println("Saved Web UI only mode; SkySafari, Alpaca, and Stellarium will be disabled.");
  }

  else if (cmd == "apdefault") {
    apSsid = "NexStar_Bridge";
    apPass = "12345678";
    apIp = "192.168.4.1";
    staRuntimeDisabled = true;
    bool ok = savePersistentSettings();
    scheduleApRestart("serial apdefault");
    Serial.printf("AP defaults saved=%d; soft AP restarting as %s / %s / %s\n",
                  ok ? 1 : 0, runtimeApSsid().c_str(), apPass.c_str(), apIp.c_str());
  }

  else if (cmd.startsWith("setsta ")) {
    String rest = line.substring(7);
    rest.trim();
    int sp = rest.indexOf(' ');
    if (sp <= 0) {
      Serial.println("Usage: setsta <ssid> <password>");
    } else {
      String ssid = rest.substring(0, sp);
      String pass = rest.substring(sp + 1);
      ssid.trim();
      pass.trim();
      staRuntimeDisabled = false;
      bool saved = saveWiFiConfig(ssid, pass);
      bool connected = false;
      if (saved) {
        WiFi.mode(WIFI_STA);
        startFallbackAP();
        connected = connectConfiguredSTA();
      }
      Serial.printf("STA save=%d connected=%d ssid=%s ip=%s status=%s\n",
                    saved ? 1 : 0, connected ? 1 : 0, staSsid.c_str(),
                    WiFi.localIP().toString().c_str(), lastWifiStatus.c_str());
    }
  }

  else if (cmd == "log") {
    Serial.println("[console] log command accepted: current status");
    Serial.printf("Firmware logging: %s\n", firmwareLoggingEnabled ? "enabled" : "disabled");
    printConsoleLogStatus();
  }

  else if (cmd == "logging" || cmd == "logging status") {
    Serial.printf("Firmware logging: %s\n", firmwareLoggingEnabled ? "enabled" : "disabled");
  }

  else if (cmd == "logging 0" || cmd == "logging off" || cmd == "logging disable") {
    bool ok = saveFirmwareLoggingEnabled(false);
    Serial.printf("Firmware logging disabled; saved=%d\n", ok ? 1 : 0);
  }

  else if (cmd == "logging 1" || cmd == "logging on" || cmd == "logging enable") {
    bool ok = saveFirmwareLoggingEnabled(true);
    Serial.printf("Firmware logging enabled; saved=%d\n", ok ? 1 : 0);
  }

  else if (cmd.startsWith("log ")) {
    String args = line.substring(4);
    args.trim();

    int sp = args.indexOf(' ');
    String lvlText = (sp >= 0) ? args.substring(0, sp) : args;
    lvlText.trim();

    int lvl = lvlText.toInt();
    if (lvl < 0) lvl = 0;
    if (lvl > 5) lvl = 5;
    LOG_LEVEL = lvl;

    Serial.printf("[console] log level applied: %d (%s)\n", LOG_LEVEL, logLevelName(LOG_LEVEL));

    if (sp >= 0) {
      String cats = args.substring(sp + 1);
      cats.trim();
      if (cats.length() > 0) {
        uint16_t mask = consoleLogMaskFromList(cats);
        LOG_SUBSYSTEM_MASK = mask;

        Serial.print("[console] log systems applied: ");
        Serial.println(cats);
        Serial.printf("[console] log system mask now: 0x%04X\n", LOG_SUBSYSTEM_MASK);
        Serial.print("[console] enabled systems now: ");
        Serial.println(consoleLogMaskNames(LOG_SUBSYSTEM_MASK));
      }
    }

    String msg = "[info][system] Serial console log command applied: level=";
    msg += logLevelName(LOG_LEVEL);
    msg += " mask=0x";
    msg += String(LOG_SUBSYSTEM_MASK, HEX);
    msg += " enabled=";
    msg += consoleLogMaskNames(LOG_SUBSYSTEM_MASK);
    msg += " command='";
    msg += line;
    msg += "'";
    addLogLine(msg);
  }

  else if (cmd == "testinit") {
    bool ok = testInit();
    Serial.printf("testinit result: %s\n", ok ? "OK" : "FAILED");
  }

  else if (cmd == "rawmount" || cmd.startsWith("rawmount ")) {
    String args = line.length() > 8 ? line.substring(8) : "";
    args.trim();
    uint16_t cycles = 10;
    bool includeAltAz = false;
    unsigned long intervalMs = 2000;
    if (args.length()) {
      int sp = args.indexOf(' ');
      String first = sp >= 0 ? args.substring(0, sp) : args;
      first.trim();
      if (first.length()) cycles = (uint16_t)first.toInt();
      if (sp >= 0) {
        String rest = args.substring(sp + 1);
        rest.trim();
        int sp2 = rest.indexOf(' ');
        String mode = sp2 >= 0 ? rest.substring(0, sp2) : rest;
        mode.trim();
        mode.toLowerCase();
        if (mode == "ez" || mode == "z" || mode == "altaz") includeAltAz = true;
        if (sp2 >= 0) {
          String interval = rest.substring(sp2 + 1);
          interval.trim();
          intervalMs = (unsigned long)interval.toInt();
        } else if (mode.length() && mode != "ez" && mode != "z" && mode != "altaz") {
          intervalMs = (unsigned long)mode.toInt();
        }
      }
    }
    runRawMountDiagnostic(Serial, cycles, includeAltAz, intervalMs);
  }

  else if (cmd == "rawgoto" || cmd.startsWith("rawgoto ")) {
    String args = line.length() > 7 ? line.substring(7) : "";
    args.trim();
    uint16_t cycles = 1;
    bool waitForCompletionFlag = true;
    unsigned long intervalMs = 2000;
    if (args.length()) {
      int sp = args.indexOf(' ');
      String first = sp >= 0 ? args.substring(0, sp) : args;
      first.trim();
      if (first.length()) cycles = (uint16_t)first.toInt();
      if (sp >= 0) {
        String rest = args.substring(sp + 1);
        rest.trim();
        int sp2 = rest.indexOf(' ');
        String mode = sp2 >= 0 ? rest.substring(0, sp2) : rest;
        mode.trim();
        mode.toLowerCase();
        if (mode == "wait" || mode == "1" || mode == "complete") waitForCompletionFlag = true;
        if (mode == "nowait" || mode == "0" || mode == "sendonly") waitForCompletionFlag = false;
        if (sp2 >= 0) {
          String interval = rest.substring(sp2 + 1);
          interval.trim();
          intervalMs = (unsigned long)interval.toInt();
        } else if (mode.length() && mode != "wait" && mode != "1" && mode != "complete" && mode != "nowait" && mode != "0" && mode != "sendonly") {
          intervalMs = (unsigned long)mode.toInt();
        }
      }
    }
    runRawGotoSelfDiagnostic(Serial, cycles, waitForCompletionFlag, intervalMs);
  }

  else if (cmd == "get") {
    double ra = 0.0;
    double dec = 0.0;
    bool ok = getRaDec(ra, dec, true);
    Serial.printf("get result: %s\n", ok ? "OK" : "FAILED");
  }

  else if (cmd == "getaltaz") {
    double alt = 0.0;
    double az = 0.0;
    bool ok = getAltAz(alt, az, true);
    Serial.printf("getaltaz result: %s\n", ok ? "OK" : "FAILED");
  }

  else if (cmd.startsWith("goto radec ")) {
    consoleQueueGotoRaDec(line.substring(11));
  }

  else if (cmd.startsWith("goto altaz ")) {
    consoleQueueGotoAltAz(line.substring(11));
  }

  else if (cmd == "rates") {
    Serial.printf("Nudge rates: %.3f %.3f %.3f %.3f activeMountPoll=%lu throttle=%lu idleMountPoll=%lu\n",
                  nudgeRateDeg[0], nudgeRateDeg[1], nudgeRateDeg[2], nudgeRateDeg[3],
                  pollIntervalMs, minClientPollIntervalMs, idlePollIntervalMs);
  }

  else if (cmd == "mountpoll") {
    printMountPollValue(Serial);
  }

  else if (cmd.startsWith("mountpoll ")) {
    long p = line.substring(10).toInt();
    bool ok = setMountPollValue(p);
    Serial.printf("mountpoll set to %lu ms%s\n", pollIntervalMs, ok ? "" : " (save failed)");
  }

  else if (cmd == "handshake" || cmd == "handshakems") {
    Serial.printf("Handshake timeout: %lu ms\n", mountHandshakeTimeoutMs);
  }

  else if (cmd.startsWith("handshake ") || cmd.startsWith("handshakems ")) {
    String arg = cmd.startsWith("handshake ") ? line.substring(10) : line.substring(12);
    arg.trim();
    long h = arg.toInt();
    if (h < 500) h = 500;
    if (h > 10000) h = 10000;
    mountHandshakeTimeoutMs = (unsigned long)h;
    bool ok = savePersistentSettings();
    Serial.printf("Handshake timeout saved as %lu ms saved=%d\n", mountHandshakeTimeoutMs, ok ? 1 : 0);
  }

  else if (cmd == "idlepoll" || cmd == "poll idle") {
    Serial.printf("Idle poll interval: %lu ms%s\n", idlePollIntervalMs, idlePollIntervalMs == 0 ? " (off)" : "");
    Serial.printf("Active mountpoll interval: %lu ms%s\n", pollIntervalMs, pollIntervalMs == 0 ? " (off)" : "");
  }

  else if (cmd.startsWith("idlepoll ") || cmd.startsWith("poll idle ")) {
    String arg = cmd.startsWith("idlepoll ") ? line.substring(9) : line.substring(10);
    arg.trim();
    long p = arg.toInt();
    if (p < 0) p = 0;
    if (p > 60000) p = 60000;
    idlePollIntervalMs = (unsigned long)p;
    resetMountPollScheduler();
    bool ok = savePersistentSettings();
    Serial.printf("Idle poll setting saved as %lu ms; active mountpoll=%lu ms saved=%d\n",
                  idlePollIntervalMs, pollIntervalMs, ok ? 1 : 0);
  }

  else if (cmd == "nudge az+") Serial.printf("nudge az+ result: %s\n", nudgeRelativeAltAz(0.0, nudgeRateDeg[1]) ? "OK" : "FAILED");
  else if (cmd == "nudge az-") Serial.printf("nudge az- result: %s\n", nudgeRelativeAltAz(0.0, -nudgeRateDeg[1]) ? "OK" : "FAILED");
  else if (cmd == "nudge alt+") Serial.printf("nudge alt+ result: %s\n", nudgeRelativeAltAz(nudgeRateDeg[1], 0.0) ? "OK" : "FAILED");
  else if (cmd == "nudge alt-") Serial.printf("nudge alt- result: %s\n", nudgeRelativeAltAz(-nudgeRateDeg[1], 0.0) ? "OK" : "FAILED");
  else if (cmd == "drain") drainMount();
  else if (cmd == "pos") Serial.println(currentStateText());
  else { Serial.print("Unknown command: "); Serial.println(line); Serial.println("Type help."); }
}
