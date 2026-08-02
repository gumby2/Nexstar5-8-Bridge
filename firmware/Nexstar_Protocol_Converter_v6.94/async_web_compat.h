#pragma once

#if defined(ESP32)
#include <Arduino.h>
#include <PsychicHttpServer.h>
#include <PsychicRequest.h>
#include <PsychicResponse.h>
#include <PsychicClient.h>
#include <vector>
#ifndef CONTENT_LENGTH_UNKNOWN
#define CONTENT_LENGTH_UNKNOWN -1
#endif

class AsyncWebCompatServer {
public:
  explicit AsyncWebCompatServer(uint16_t port);
  ~AsyncWebCompatServer();

  void on(const char* uri, int method, void (*handler)());
  void on(const char* uri, void (*handler)()) { on(uri, HTTP_GET, handler); }
  void onNotFound(void (*handler)());
  esp_err_t begin();
  esp_err_t end();
  void handleClient() {}

  String uri() const;
  int method() const;
  bool hasArg(const String& name) const;
  String arg(const String& name) const;
  String arg(int index) const;
  String argName(int index) const;
  int args() const;
  void sendHeader(const String& name, const String& value, bool first = false);
  void setContentLength(int64_t length);
  void send(int code, const String& contentType = String(), const String& content = String());
  void send(int code, const String& contentType, const uint8_t* content, size_t length);
  void sendContent(const String& content);
  void sendContent_P(const char* content);
  void sendContent_P(const char* content, size_t length);
  void collectHeaders(const char**, size_t) {}
  String hostHeader() const;
  PsychicClient& client();

private:
  PsychicHttpServer server_;
  PsychicRequest* request_ = nullptr;
  PsychicResponse* response_ = nullptr;
  std::vector<std::pair<String, String>> args_;
  void (*notFoundHandler_)() = nullptr;
  bool streaming_ = false;
  bool headersSent_ = false;

  void invoke(PsychicRequest* request, PsychicResponse* response, void (*handler)());
  void parseArgs();
  void finish();
  static String decode(const String& value);
};
#endif
