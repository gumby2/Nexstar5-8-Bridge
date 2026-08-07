#include "mount_transport.h"

#include "logging.h"
#include "network_services.h"
#include "position_cache.h"
#include "settings.h"

#if defined(ESP8266)
  #include <SoftwareSerial.h>
#endif

#if defined(ESP32) && !defined(CONFIG_IDF_TARGET_ESP32S2)
  #ifndef ENABLE_CLASSIC_BT
    #define ENABLE_CLASSIC_BT 1
  #endif
  #define HAS_MOUNT_TRANSPORT_CLASSIC_BT ENABLE_CLASSIC_BT
#else
  #define HAS_MOUNT_TRANSPORT_CLASSIC_BT 0
#endif

#if defined(ESP8266)
  SoftwareSerial MountSerial(MOUNT_RX_PIN, MOUNT_TX_PIN);
  const char* BOARD_NAME = "ESP8266 ESP-12E / D1 mini style";
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
  HardwareSerial MountSerial(1);
  const char* BOARD_NAME = "ESP32-S2";
#else
  HardwareSerial MountSerial(2);
  const char* BOARD_NAME = "Regular ESP32";
#endif

extern const unsigned long MOUNT_BAUD = 9600;
extern const unsigned long READ_TIMEOUT_MS = 3000;
extern const unsigned long GOTO_TIMEOUT_MS = 180000;

extern const unsigned long COOLDOWN_MS = 100;
extern const unsigned long AFTER_HANDSHAKE_TX_DELAY_MS = 50;
extern const unsigned long AFTER_COMMAND_TX_DELAY_MS = 50;
extern const unsigned long BEFORE_PAYLOAD_DELAY_MS = 100;
static const unsigned long MIN_MOUNT_COMMAND_GAP_MS = 1000;

unsigned long lastMountResponseMs = 0;
unsigned long lastMountFaultMs = 0;
bool mountCommFault = false;
String lastMountFault = "No mount communication yet";
bool mountBusy = false;
bool suppressNextMountFault = false;
static unsigned long lastMountCommandEndMs = 0;
static uint8_t mountFaultStreak = 0;
static const uint8_t MOUNT_FAULT_STREAK_LIMIT = 3;
static const unsigned long MOUNT_FAULT_STALE_RESPONSE_MS = 30000;

void mountTransportBegin() {
#if defined(ESP8266)
  MountSerial.begin(MOUNT_BAUD);
#else
  MountSerial.begin(MOUNT_BAUD, SERIAL_8N1, MOUNT_RX_PIN, MOUNT_TX_PIN);
#endif
}

int mountRawAvailable() {
  return MountSerial.available();
}

int mountReadRawByte() {
  return MountSerial.read();
}

void mountWriteRawByte(uint8_t b) {
  MountSerial.write(b);
}

void mountFlush() {
  MountSerial.flush();
}

void markMountResponse() {
  lastMountResponseMs = millis();
  mountCommFault = false;
  mountFaultStreak = 0;
  lastMountFault = "";
  backgroundPollFailCount = 0;
}

void markMountFault(const String &reason) {
  lastMountFaultMs = millis();
  lastMountFault = reason;

  if (suppressNextMountFault) {
    LOGD("Suppressed transient mount fault: %s", reason.c_str());
    suppressNextMountFault = false;
    return;
  }

  if (mountFaultStreak < 255) mountFaultStreak++;
  unsigned long now = millis();
  bool responseStale = (lastMountResponseMs == 0) || (now - lastMountResponseMs > MOUNT_FAULT_STALE_RESPONSE_MS);
  if (mountFaultStreak >= MOUNT_FAULT_STREAK_LIMIT || responseStale) {
    mountCommFault = true;
    LOG_MOUNT_W("Persistent mount fault %u/%u: %s", mountFaultStreak, MOUNT_FAULT_STREAK_LIMIT, reason.c_str());
  } else {
    LOG_MOUNT_W("Transient mount fault %u/%u: %s", mountFaultStreak, MOUNT_FAULT_STREAK_LIMIT, reason.c_str());
  }
}

bool mountAlive() {
  if (lastMountResponseMs == 0) return false;
  if (mountCommFault) return false;
  return millis() - lastMountResponseMs <= MOUNT_FAULT_STALE_RESPONSE_MS;
}

unsigned long mountLastResponseAge() {
  if (lastMountResponseMs == 0) return 0;
  return millis() - lastMountResponseMs;
}

bool isPrintableByte(uint8_t b) {
  return b >= 32 && b <= 126;
}

void safeDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    delay(1);
    yield();
  }
}

void printRxByte(uint8_t b) {
  LOG_MOUNT_T("RX MOUNT 0x%02X '%c'", b, isPrintableByte(b) ? b : '.');
}

void printTxByte(uint8_t b) {
  LOG_MOUNT_T("TX MOUNT 0x%02X '%c'", b, isPrintableByte(b) ? b : '.');
}

void mountWriteByte(uint8_t b) {
  printTxByte(b);
  MountSerial.write(b);
  MountSerial.flush();
}

void mountWriteBE16(int16_t value) {
  mountWriteByte((uint8_t)((value >> 8) & 0xFF));
  mountWriteByte((uint8_t)(value & 0xFF));
}

bool mountReadByte(uint8_t &b, unsigned long timeoutMs) {
  unsigned long start = millis();

  while (millis() - start < timeoutMs) {
    if (MountSerial.available()) {
      b = MountSerial.read();
      printRxByte(b);
      markMountResponse();
      return true;
    }

    serviceNetworkDuringMountWait();
  }

  String reason = "mountReadByte timeout after " + String(timeoutMs) + " ms";
  LOGW("%s", reason.c_str());
  markMountFault(reason);
  noteMountCommandFailure(reason.c_str());
  return false;
}

bool mountReadByteQuiet(uint8_t &b, unsigned long timeoutMs) {
  unsigned long start = millis();

  while (millis() - start < timeoutMs) {
    if (MountSerial.available()) {
      b = MountSerial.read();
      printRxByte(b);
      markMountResponse();
      return true;
    }

    serviceNetworkDuringMountWait();
  }

  return false;
}

bool mountReadExact(uint8_t* buf, size_t len, unsigned long timeoutMs) {
  size_t got = 0;
  unsigned long start = millis();

  while (got < len && millis() - start < timeoutMs) {
    if (MountSerial.available()) {
      uint8_t b = MountSerial.read();
      buf[got++] = b;
      printRxByte(b);
      markMountResponse();
      start = millis();
    } else {
      serviceNetworkDuringMountWait();
    }
  }

  if (got != len) {
    String reason = "mountReadExact timeout: expected " + String((unsigned)len) + " got " + String((unsigned)got);
    LOG_MOUNT_W("%s", reason.c_str());
    markMountFault(reason);
    noteMountCommandFailure(reason.c_str());
    return false;
  }

  return true;
}

void drainMount() {
  int count = 0;
  while (MountSerial.available()) {
    uint8_t b = MountSerial.read();
    LOG_MOUNT_I("[DRAIN] 0x%02X '%c'", b, isPrintableByte(b) ? b : '.');
    markMountResponse();
    count++;
  }
  LOG_MOUNT_I("Drained %d byte(s)", count);
}

bool beginMountCommand(const char* name) {
  if (mountBusy) {
    LOG_MOUNT_D("Blocked mount command '%s': mountBusy=true", name);
    return false;
  }

  mountBusy = true;
  unsigned long nowMs = millis();
  if (lastMountCommandEndMs && nowMs - lastMountCommandEndMs < MIN_MOUNT_COMMAND_GAP_MS) {
    unsigned long waitMs = MIN_MOUNT_COMMAND_GAP_MS - (nowMs - lastMountCommandEndMs);
    LOG_MOUNT_D("Command gap before '%s': waiting %lu ms", name, waitMs);
    safeDelay(waitMs);
  }
  LOG_MOUNT_D("Begin mount command: %s", name);
  return true;
}

void endMountCommand(const char* name) {
  LOG_MOUNT_D("End mount command: %s", name);
  lastMountCommandEndMs = millis();
  mountBusy = false;
}

bool nexstarHandshakeLocked(const char* name) {
  uint8_t maxAttempts = (strcmp(name, "GET RA/Dec E") == 0) ? 1 : 2;
  unsigned long timeoutMs = mountHandshakeTimeoutMs;
  if (strcmp(name, "GET RA/Dec E") == 0 && timeoutMs > 1000UL) timeoutMs = 1000UL;
  for (uint8_t attempt = 0; attempt < maxAttempts; attempt++) {
    safeDelay(COOLDOWN_MS);

    LOG_MOUNT_D("%s: sending handshake '?' attempt=%u", name, (unsigned)(attempt + 1));
    mountWriteByte('?');

    safeDelay(AFTER_HANDSHAKE_TX_DELAY_MS);

    unsigned long start = millis();
    uint8_t ignored = 0;
    while (millis() - start < timeoutMs) {
      uint8_t resp = 0;
      if (!mountReadByteQuiet(resp, 250)) continue;

      if (resp == '#') {
        if (ignored > 0) {
          LOG_MOUNT_W("%s: handshake OK after ignoring %u stale byte(s)", name, (unsigned)ignored);
        } else {
          LOG_MOUNT_D("%s: handshake OK", name);
        }
        markMountResponse();
        safeDelay(COOLDOWN_MS);
        return true;
      }

      ignored++;
      LOG_MOUNT_W("%s: ignored stale byte before handshake '#': 0x%02X '%c'",
                  name,
                  resp,
                  isPrintableByte(resp) ? resp : '.');
    }

    LOG_MOUNT_W("%s: handshake attempt %u timed out waiting for '#'",
                name,
                (unsigned)(attempt + 1));
  }

  String reason = String(name) + ": handshake failed, no '#'";
  if (suppressNextMountFault) LOGW("%s", reason.c_str());
  else LOGE("%s", reason.c_str());
  markMountFault(reason);
  noteMountCommandFailure(reason.c_str());
  return false;
}

bool waitForNexStarCompletion(const char* name, unsigned long timeoutMs) {
  unsigned long start = millis();
  LOG_MOUNT_D("%s: waiting for NexStar completion '@'", name);

  while (millis() - start < timeoutMs) {
    uint8_t b = 0;
    if (mountReadByteQuiet(b, 250)) {
      if (b == '@') {
        LOG_MOUNT_I("%s: completion '@' received", name);
        markMountResponse();
        drainMountAfterCompletion();
        return true;
      }
      LOG_MOUNT_W("%s: ignored byte while waiting for '@': 0x%02X '%c'",
                  name, b, isPrintableByte(b) ? b : '.');
    }

#if HAS_MOUNT_TRANSPORT_CLASSIC_BT
    // Keep the Bluetooth stack alive, but do not process new SkySafari commands
    // while the original NexStar mount is completing this single-command move.
    if (bridgeMode == BRIDGE_MODE_BT_MIN_WEB) {
      delay(1);
      yield();
    } else
#endif
    {
      serviceNetworkDuringMountWait();
    }
  }

  String reason = String(name) + ": timed out waiting for completion '@'";
  LOG_MOUNT_W("%s", reason.c_str());
  markMountFault(reason);
  noteMountCommandFailure(reason.c_str());
  return false;
}

uint16_t drainMountAfterCompletion(unsigned long maxMs, unsigned long quietMs) {
  const unsigned long startMs = millis();
  unsigned long lastByteMs = startMs;
  uint16_t count = 0;
  uint8_t sample[8] = {0};
  uint8_t sampleCount = 0;

  while (millis() - startMs < maxMs && millis() - lastByteMs < quietMs) {
    if (MountSerial.available()) {
      const uint8_t b = MountSerial.read();
      printRxByte(b);
      markMountResponse();
      if (sampleCount < sizeof(sample)) sample[sampleCount++] = b;
      count++;
      lastByteMs = millis();
    } else {
      serviceNetworkDuringMountWait();
      delay(1);
      yield();
    }
  }

  if (count) {
    char bytes[8 * 3 + 1] = "";
    size_t used = 0;
    for (uint8_t i = 0; i < sampleCount && used + 3 < sizeof(bytes); i++) {
      used += snprintf(bytes + used, sizeof(bytes) - used, "%02X%s",
                       sample[i], i + 1 < sampleCount ? " " : "");
    }
    LOG_MOUNT_W("Post-completion drain discarded %u byte(s), first=%s%s",
                (unsigned)count,
                bytes,
                count > sampleCount ? " ..." : "");
  }
  return count;
}

static bool rawDiagReadByte(uint8_t &b, unsigned long timeoutMs, unsigned long &elapsedMs) {
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (MountSerial.available()) {
      b = MountSerial.read();
      elapsedMs = millis() - start;
      printRxByte(b);
      markMountResponse();
      return true;
    }
    delay(1);
    yield();
  }
  elapsedMs = millis() - start;
  return false;
}

static void rawDiagPrintByte(Print &out, const char *prefix, uint8_t b, unsigned long elapsedMs) {
  out.printf("  %s +%lu ms 0x%02X '%c'\n",
             prefix,
             elapsedMs,
             b,
             isPrintableByte(b) ? b : '.');
}

static bool rawDiagHandshake(Print &out, const char *label, uint16_t cycle, uint8_t &ignored) {
  ignored = 0;
  unsigned long start = millis();
  out.printf("[%u] TX ? for %s\n", cycle, label);
  mountWriteByte('?');

  while (millis() - start < mountHandshakeTimeoutMs) {
    uint8_t b = 0;
    unsigned long elapsed = 0;
    if (!rawDiagReadByte(b, 250, elapsed)) continue;
    unsigned long totalElapsed = millis() - start;
    rawDiagPrintByte(out, "RX handshake", b, totalElapsed);
    if (b == '#') {
      out.printf("[%u] handshake OK ignored=%u elapsed=%lu ms\n",
                 cycle, (unsigned)ignored, totalElapsed);
      return true;
    }
    ignored++;
  }

  out.printf("[%u] handshake FAIL no '#' ignored=%u timeout=%lu ms\n",
             cycle, (unsigned)ignored, mountHandshakeTimeoutMs);
  return false;
}

static bool rawDiagReadPayload(Print &out, const char *label, uint16_t cycle, uint8_t command) {
  uint8_t buf[4] = {0, 0, 0, 0};
  size_t got = 0;
  unsigned long start = millis();
  unsigned long lastByteStart = millis();

  out.printf("[%u] TX %c for %s\n", cycle, command, label);
  mountWriteByte(command);

  while (got < sizeof(buf) && millis() - lastByteStart < READ_TIMEOUT_MS) {
    if (MountSerial.available()) {
      uint8_t b = MountSerial.read();
      buf[got++] = b;
      lastByteStart = millis();
      printRxByte(b);
      markMountResponse();
      rawDiagPrintByte(out, "RX payload", b, millis() - start);
    } else {
      delay(1);
      yield();
    }
  }

  if (got == sizeof(buf)) {
    out.printf("[%u] %s OK bytes=%02X %02X %02X %02X elapsed=%lu ms\n",
               cycle, label, buf[0], buf[1], buf[2], buf[3], millis() - start);
    return true;
  }

  out.printf("[%u] %s FAIL expected=4 got=%u elapsed=%lu ms bytes=%02X %02X %02X %02X\n",
             cycle, label, (unsigned)got, millis() - start, buf[0], buf[1], buf[2], buf[3]);
  return false;
}

static bool rawDiagReadPayloadTo(Print &out, const char *label, uint16_t cycle, uint8_t command, uint8_t *buf, size_t len) {
  size_t got = 0;
  unsigned long start = millis();
  unsigned long lastByteStart = millis();

  out.printf("[%u] TX %c for %s\n", cycle, command, label);
  mountWriteByte(command);

  while (got < len && millis() - lastByteStart < READ_TIMEOUT_MS) {
    if (MountSerial.available()) {
      uint8_t b = MountSerial.read();
      buf[got++] = b;
      lastByteStart = millis();
      printRxByte(b);
      markMountResponse();
      rawDiagPrintByte(out, "RX payload", b, millis() - start);
    } else {
      delay(1);
      yield();
    }
  }

  if (got == len) {
    out.printf("[%u] %s OK bytes=%02X %02X %02X %02X elapsed=%lu ms\n",
               cycle, label, buf[0], buf[1], buf[2], buf[3], millis() - start);
    return true;
  }

  out.printf("[%u] %s FAIL expected=%u got=%u elapsed=%lu ms\n",
             cycle, label, (unsigned)len, (unsigned)got, millis() - start);
  return false;
}

static uint8_t rawDiagDrainAfterCompletion(Print &out, unsigned long settleMs) {
  unsigned long start = millis();
  unsigned long lastSeen = start;
  uint8_t count = 0;
  while (millis() - start < settleMs) {
    if (MountSerial.available()) {
      uint8_t b = MountSerial.read();
      lastSeen = millis();
      count++;
      printRxByte(b);
      markMountResponse();
      out.printf("  post-completion drain +%lu ms 0x%02X '%c'\n",
                 lastSeen - start,
                 b,
                 isPrintableByte(b) ? b : '.');
    } else {
      delay(1);
      yield();
    }
  }
  if (count) out.printf("post-completion drain removed %u byte(s)\n", (unsigned)count);
  return count;
}

void runRawMountDiagnostic(Print &out, uint16_t cycles, bool includeAltAz, unsigned long intervalMs) {
  if (cycles < 1) cycles = 1;
  if (cycles > 200) cycles = 200;
  if (intervalMs > 10000) intervalMs = 10000;

  const char* commandName = "rawmount diagnostic";
  if (!beginMountCommand(commandName)) {
    out.println("rawmount refused: mountBusy=true");
    return;
  }
  uint16_t handshakeOk = 0;
  uint16_t handshakeFail = 0;
  uint16_t eOk = 0;
  uint16_t eFail = 0;
  uint16_t zOk = 0;
  uint16_t zFail = 0;
  uint32_t ignoredTotal = 0;

  out.printf("rawmount start cycles=%u includeAltAz=%u interval=%lu ms handshakeTimeout=%lu ms readTimeout=%lu ms\n",
             (unsigned)cycles,
             includeAltAz ? 1 : 0,
             intervalMs,
             mountHandshakeTimeoutMs,
             READ_TIMEOUT_MS);
  out.println("No automatic drain is performed; preAvailable reports bytes already waiting before each cycle.");

  for (uint16_t i = 1; i <= cycles; i++) {
    safeDelay(MIN_MOUNT_COMMAND_GAP_MS);
    int preAvailable = MountSerial.available();
    out.printf("[%u] preAvailable=%d\n", i, preAvailable);

    uint8_t ignored = 0;
    bool hsOk = rawDiagHandshake(out, "RA/Dec E", i, ignored);
    ignoredTotal += ignored;
    if (hsOk) {
      handshakeOk++;
      if (rawDiagReadPayload(out, "RA/Dec E", i, 'E')) eOk++;
      else eFail++;
    } else {
      handshakeFail++;
      eFail++;
    }

    if (includeAltAz) {
      uint8_t zIgnored = 0;
      bool zHsOk = rawDiagHandshake(out, "Alt/Az Z", i, zIgnored);
      ignoredTotal += zIgnored;
      if (zHsOk) {
        handshakeOk++;
        if (rawDiagReadPayload(out, "Alt/Az Z", i, 'Z')) zOk++;
        else zFail++;
      } else {
        handshakeFail++;
        zFail++;
      }
    }

    if (i < cycles && intervalMs > 0) {
      unsigned long waitStart = millis();
      while (millis() - waitStart < intervalMs) {
        delay(1);
        yield();
      }
    }
  }

  endMountCommand(commandName);
  out.printf("rawmount summary cycles=%u handshakeOK=%u handshakeFail=%u ignoredHandshakeBytes=%lu E_OK=%u E_FAIL=%u Z_OK=%u Z_FAIL=%u\n",
             (unsigned)cycles,
             (unsigned)handshakeOk,
             (unsigned)handshakeFail,
             (unsigned long)ignoredTotal,
             (unsigned)eOk,
             (unsigned)eFail,
             (unsigned)zOk,
             (unsigned)zFail);
}

void runRawGotoSelfDiagnostic(Print &out, uint16_t cycles, bool waitForCompletionFlag, unsigned long intervalMs) {
  if (cycles < 1) cycles = 1;
  if (cycles > 50) cycles = 50;
  if (intervalMs > 10000) intervalMs = 10000;

  const char* commandName = "rawgoto diagnostic";
  if (!beginMountCommand(commandName)) {
    out.println("rawgoto refused: mountBusy=true");
    return;
  }
  uint16_t readOk = 0;
  uint16_t readFail = 0;
  uint16_t gotoHandshakeOk = 0;
  uint16_t gotoHandshakeFail = 0;
  uint16_t gotoSent = 0;
  uint16_t completionOk = 0;
  uint16_t completionFail = 0;
  uint32_t ignoredTotal = 0;

  const unsigned long rawGotoCompletionTimeoutMs = 30000;

  out.printf("rawgoto start cycles=%u wait=%u interval=%lu ms handshakeTimeout=%lu ms readTimeout=%lu ms completionTimeout=%lu ms\n",
             (unsigned)cycles,
             waitForCompletionFlag ? 1 : 0,
             intervalMs,
             mountHandshakeTimeoutMs,
             READ_TIMEOUT_MS,
             rawGotoCompletionTimeoutMs);
  out.println("Each cycle reads current raw E bytes, then sends R with those exact same 4 bytes.");

  for (uint16_t i = 1; i <= cycles; i++) {
    safeDelay(MIN_MOUNT_COMMAND_GAP_MS);
    uint8_t payload[4] = {0, 0, 0, 0};
    int preAvailable = MountSerial.available();
    out.printf("[%u] preAvailable=%d\n", i, preAvailable);

    uint8_t ignored = 0;
    bool eHsOk = rawDiagHandshake(out, "read current RA/Dec E", i, ignored);
    ignoredTotal += ignored;
    if (!eHsOk) {
      readFail++;
      out.printf("[%u] rawgoto skipped: current RA/Dec read handshake failed\n", i);
    } else if (!rawDiagReadPayloadTo(out, "current RA/Dec E", i, 'E', payload, sizeof(payload))) {
      readFail++;
      out.printf("[%u] rawgoto skipped: current RA/Dec read payload failed\n", i);
    } else {
      readOk++;
      out.printf("[%u] current raw target for self-GOTO: RA=%02X %02X DEC=%02X %02X\n",
                 i, payload[0], payload[1], payload[2], payload[3]);

      safeDelay(MIN_MOUNT_COMMAND_GAP_MS);

      uint8_t gotoIgnored = 0;
      bool rHsOk = rawDiagHandshake(out, "self GOTO RA/Dec R", i, gotoIgnored);
      ignoredTotal += gotoIgnored;
      if (!rHsOk) {
        gotoHandshakeFail++;
        out.printf("[%u] self-GOTO FAIL: R handshake failed\n", i);
      } else {
        gotoHandshakeOk++;
        out.printf("[%u] TX R self-GOTO payload\n", i);
        mountWriteByte('R');
        safeDelay(AFTER_COMMAND_TX_DELAY_MS);
        safeDelay(BEFORE_PAYLOAD_DELAY_MS);
        for (uint8_t j = 0; j < sizeof(payload); j++) {
          mountWriteByte(payload[j]);
        }
        gotoSent++;
        out.printf("[%u] self-GOTO R sent bytes=%02X %02X %02X %02X\n",
                   i, payload[0], payload[1], payload[2], payload[3]);

        if (waitForCompletionFlag) {
          if (waitForNexStarCompletion("rawgoto self-GOTO R", rawGotoCompletionTimeoutMs)) {
            completionOk++;
            rawDiagDrainAfterCompletion(out, 750);
            deferMountPolling(10000, "rawgoto self-GOTO completed");
            out.printf("[%u] self-GOTO completion OK\n", i);
          } else {
            completionFail++;
            out.printf("[%u] self-GOTO completion FAIL\n", i);
          }
        }
      }
    }

    if (i < cycles && intervalMs > 0) {
      unsigned long waitStart = millis();
      while (millis() - waitStart < intervalMs) {
        delay(1);
        yield();
      }
    }
  }

  endMountCommand(commandName);
  out.printf("rawgoto summary cycles=%u readOK=%u readFail=%u gotoHandshakeOK=%u gotoHandshakeFail=%u gotoSent=%u completionOK=%u completionFail=%u ignoredHandshakeBytes=%lu\n",
             (unsigned)cycles,
             (unsigned)readOk,
             (unsigned)readFail,
             (unsigned)gotoHandshakeOk,
             (unsigned)gotoHandshakeFail,
             (unsigned)gotoSent,
             (unsigned)completionOk,
             (unsigned)completionFail,
             (unsigned long)ignoredTotal);
}
