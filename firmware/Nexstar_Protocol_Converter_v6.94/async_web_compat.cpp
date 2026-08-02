#include "async_web_compat.h"

#define NEXSTAR_DIAGNOSTIC_LIGHT 1

#if defined(ESP32)

extern thread_local AsyncWebCompatServer* activeServer;
static uint32_t asyncHttpRequestSequence = 0;

struct AsyncWebCompatContext {
  PsychicRequest* request = nullptr;
  PsychicResponse* response = nullptr;
  std::vector<std::pair<String, String>> args;
  bool streaming = false;
  bool headersSent = false;
};

static thread_local AsyncWebCompatContext asyncContext;

AsyncWebCompatServer::AsyncWebCompatServer(uint16_t port) : server_(port) {
  // PsychicHttp uses an internal control socket per server instance. Its
  // default control port collides when the Web UI and Alpaca servers coexist.
  server_.config.ctrl_port = (uint16_t)(port + 10000U);
}

AsyncWebCompatServer::~AsyncWebCompatServer() { end(); }

void AsyncWebCompatServer::invoke(PsychicRequest* request, PsychicResponse* response, void (*handler)()) {
  // Existing handlers use the legacy `server` macro. Bind it to the actual
  // async listener that received this request; the old loop-time assignment
  // is not safe once callbacks run outside loop().
  activeServer = this;
  const uint32_t requestId = ++asyncHttpRequestSequence;
  const unsigned long requestStartMs = millis();
#if !defined(NEXSTAR_DIAGNOSTIC_LIGHT)
  Serial.printf("[ASYNC_HTTP] BEGIN id=%lu port=%u method=%d path=%s\n",
                (unsigned long)requestId,
                (unsigned)server_.getPort(),
                (int)request->method(),
                request->path().c_str());
#endif
  asyncContext.request = request;
  asyncContext.response = response;
  asyncContext.streaming = false;
  asyncContext.headersSent = false;
  asyncContext.args.clear();
  parseArgs();
  if (handler) handler();
  finish();
#if !defined(NEXSTAR_DIAGNOSTIC_LIGHT)
  Serial.printf("[ASYNC_HTTP] END id=%lu port=%u ms=%lu code=%d\n",
                (unsigned long)requestId,
                (unsigned)server_.getPort(),
                (unsigned long)(millis() - requestStartMs),
                response->getCode());
#endif
  asyncContext.request = nullptr;
  asyncContext.response = nullptr;
  asyncContext.args.clear();
}

void AsyncWebCompatServer::on(const char* uri, int method, void (*handler)()) {
  server_.on(uri, method, [this, handler](PsychicRequest* request, PsychicResponse* response) -> esp_err_t {
    invoke(request, response, handler);
    return ESP_OK;
  });
}

void AsyncWebCompatServer::onNotFound(void (*handler)()) {
  notFoundHandler_ = handler;
  server_.onNotFound([this](PsychicRequest* request, PsychicResponse* response) -> esp_err_t {
    invoke(request, response, notFoundHandler_);
    return ESP_OK;
  });
}

esp_err_t AsyncWebCompatServer::begin() { return server_.begin(); }
esp_err_t AsyncWebCompatServer::end() { return server_.end(); }

String AsyncWebCompatServer::uri() const { return asyncContext.request ? asyncContext.request->uri() : String(); }
int AsyncWebCompatServer::method() const { return asyncContext.request ? (int)asyncContext.request->method() : HTTP_ANY; }

bool AsyncWebCompatServer::hasArg(const String& name) const {
  for (const auto& item : asyncContext.args) if (item.first == name) return true;
  return false;
}

String AsyncWebCompatServer::arg(const String& name) const {
  for (const auto& item : asyncContext.args) if (item.first == name) return item.second;
  return String();
}

String AsyncWebCompatServer::arg(int index) const {
  return (index >= 0 && index < (int)asyncContext.args.size()) ? asyncContext.args[(size_t)index].second : String();
}

String AsyncWebCompatServer::argName(int index) const {
  return (index >= 0 && index < (int)asyncContext.args.size()) ? asyncContext.args[(size_t)index].first : String();
}

int AsyncWebCompatServer::args() const { return (int)asyncContext.args.size(); }

void AsyncWebCompatServer::sendHeader(const String& name, const String& value, bool) {
  if (asyncContext.response) asyncContext.response->addHeader(name.c_str(), value.c_str());
}

void AsyncWebCompatServer::setContentLength(int64_t length) {
  if (asyncContext.response) asyncContext.response->setContentLength(length);
}

void AsyncWebCompatServer::send(int code, const String& contentType, const String& content) {
  if (!asyncContext.response) return;
  asyncContext.response->setCode(code);
  if (contentType.length()) asyncContext.response->setContentType(contentType.c_str());
  if (content.length()) {
    esp_err_t err = asyncContext.response->send(contentType.c_str(), content.c_str());
    if (err != ESP_OK) Serial.printf("[ASYNC_HTTP] SEND_ERROR code=%d err=%d\n", code, (int)err);
    asyncContext.headersSent = true;
    asyncContext.streaming = false;
  } else {
    asyncContext.response->sendHeaders();
    asyncContext.headersSent = true;
    asyncContext.streaming = true;
  }
}

void AsyncWebCompatServer::send(int code, const String& contentType, const uint8_t* content, size_t length) {
  if (!asyncContext.response) return;
  asyncContext.response->setCode(code);
  asyncContext.response->setContentType(contentType.c_str());
  esp_err_t err = asyncContext.response->send(code, contentType.c_str(), content, length);
  if (err != ESP_OK) Serial.printf("[ASYNC_HTTP] SEND_ERROR code=%d len=%u err=%d\n", code, (unsigned)length, (int)err);
  asyncContext.headersSent = true;
  asyncContext.streaming = false;
}

void AsyncWebCompatServer::sendContent(const String& content) {
  if (!asyncContext.response) return;
  if (!asyncContext.headersSent) {
    asyncContext.response->setCode(200);
    asyncContext.response->setContentType("text/plain");
    asyncContext.response->sendHeaders();
    asyncContext.headersSent = true;
    asyncContext.streaming = true;
  }
  if (content.length()) {
    esp_err_t err = asyncContext.response->sendChunk((uint8_t*)content.c_str(), content.length());
    if (err != ESP_OK) Serial.printf("[ASYNC_HTTP] CHUNK_ERROR len=%u err=%d\n", (unsigned)content.length(), (int)err);
  } else if (asyncContext.streaming) {
    asyncContext.response->finishChunking();
    asyncContext.streaming = false;
  }
}

void AsyncWebCompatServer::sendContent_P(const char* content) { sendContent(String(content ? content : "")); }

void AsyncWebCompatServer::sendContent_P(const char* content, size_t length) {
  if (!asyncContext.response || !content) return;
  if (!asyncContext.headersSent) {
    asyncContext.response->setCode(200);
    asyncContext.response->setContentType("text/html");
    asyncContext.response->sendHeaders();
    asyncContext.headersSent = true;
    asyncContext.streaming = true;
  }
  if (length) {
    esp_err_t err = asyncContext.response->sendChunk((uint8_t*)content, length);
    if (err != ESP_OK) Serial.printf("[ASYNC_HTTP] CHUNK_ERROR len=%u err=%d\n", (unsigned)length, (int)err);
  }
}

String AsyncWebCompatServer::hostHeader() const {
  return asyncContext.request ? asyncContext.request->host() : String();
}

PsychicClient& AsyncWebCompatServer::client() {
  return *asyncContext.request->client();
}

String AsyncWebCompatServer::decode(const String& value) {
  String out;
  for (size_t i = 0; i < value.length(); ++i) {
    char c = value[i];
    if (c == '+') out += ' ';
    else if (c == '%' && i + 2 < value.length()) {
      char hex[3] = { value[i + 1], value[i + 2], 0 };
      out += (char)strtol(hex, nullptr, 16);
      i += 2;
    } else out += c;
  }
  return out;
}

void AsyncWebCompatServer::parseArgs() {
  if (!asyncContext.request) return;
  String query = asyncContext.request->query();
  int start = 0;
  while (start <= (int)query.length()) {
    int amp = query.indexOf('&', start);
    if (amp < 0) amp = query.length();
    String part = query.substring(start, amp);
    int eq = part.indexOf('=');
    if (eq >= 0) asyncContext.args.push_back({decode(part.substring(0, eq)), decode(part.substring(eq + 1))});
    else if (part.length()) asyncContext.args.push_back({decode(part), String()});
    if (amp >= (int)query.length()) break;
    start = amp + 1;
  }
}

void AsyncWebCompatServer::finish() {
  if (asyncContext.streaming && asyncContext.response) {
    esp_err_t err = asyncContext.response->finishChunking();
    if (err != ESP_OK) Serial.printf("[ASYNC_HTTP] FINISH_ERROR err=%d\n", (int)err);
  }
  asyncContext.streaming = false;
}

#endif
