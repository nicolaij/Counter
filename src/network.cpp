#include "network.h"

#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <SPIFFSEditor.h>
#include <ESPmDNS.h>

#include "synctime.h"

DynamicJsonDocument jconf(8192);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

// flag to use from web update to reboot the ESP
bool shouldReboot = false;

const char *hostName = "Woody";

#define CONFIG_FILE "/config.json"

int32_t limit_position[3];
int32_t home_correction[3];
int32_t tool_correction[3];
int32_t safety_zone[3];
int maximum_speed;
int minimum_speed;
int32_t jog_speed;

extern volatile int32_t current_position[3];

extern uint8_t log_pos;

extern int32_t process_mode;

uint8_t waitcommand = 0;

String status_text;

synctime st;

extern int num_ee_block;

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  static char inbuff[512];

  if (type == WS_EVT_CONNECT)
  {
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    // client->printf("Hello Client %u :)", client->id());
    // client->ping();
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
  }
  else if (type == WS_EVT_ERROR)
  {
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
  }
  else if (type == WS_EVT_PONG)
  {
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), (uint32_t)len, ((uint32_t)len) ? (char *)data : "");
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;

    // Serial.printf("ws[%s][%u] frame[%u] infolen[%llu], len[%zu], index:[%llu], final:[%u]\n", server->url(), client->id(), info->num, info->len, len, info->index, info->final);

    if (info->index + len < sizeof(inbuff))
    {
      memcpy(&inbuff[info->index], data, len);

      // Serial.print("message:"); Serial.println(inbuff);

      if (info->index + len == info->len)
      {
        /*
        if (info->opcode == WS_TEXT)
        {
          StaticJsonDocument<128 * LOG_LEN> jdoc;
          bool send = false;
          char output[128 * LOG_LEN];

          // Deserialize the JSON document
          DeserializationError error = deserializeJson(jdoc, inbuff);
          if (error)
            Serial.println(F("Failed parse websocket message!"));
          else
          {
            if (jdoc.containsKey("text"))
            {
              if (jdoc["text"] == "?")
              {
                jdoc.clear();
                jdoc["text"] = "";
                for (uint8_t i = 0; i < LOG_LEN; i++)
                {
                  if (log_last[(log_pos - i) & (LOG_LEN - 1)][0] != 0)
                  {
                    jdoc["text"] = String(log_last[(log_pos - i) & (LOG_LEN - 1)]) + "\n" + jdoc["text"].as<String>();
                  }
                  else
                    break;
                }
                serializeJson(jdoc, output);
                client->text(output);
                return;
              }
            }

            if (jdoc.containsKey("A"))
            {
              if (jdoc["A"] == "?")
              {
                jdoc["A"] = current_position[0];
                send = true;
              }
              else if (jdoc["A"] == "minus")
              {
                gojog.axis = 1;
                gojog.position = jdoc["step"];
                gojog.dir = -1;
                gojog.speed = jog_speed;
              }
              else if (jdoc["A"] == "plus")
              {
                gojog.axis = 1;
                gojog.position = jdoc["step"];
                gojog.dir = 1;
                gojog.speed = jog_speed;
              }
              else if (jdoc["A"] == "zero")
              {
                tool_correction[0] = current_position[0];
                current_position[0] = 0;
                jconf["tool_correctionX1"] = tool_correction[0];
              }
              else if (jdoc["A"] == "on")
              {
                startAC(10);
              }
              else if (jdoc["A"] == "off")
              {
                startAC(-10);
              }
            }

            if (jdoc.containsKey("B"))
            {
              if (jdoc["B"] == "?")
              {
                jdoc["B"] = current_position[1];
                send = true;
              }
              else if (jdoc["B"] == "minus")
              {
                gojog.axis = 2;
                gojog.position = jdoc["step"];
                gojog.dir = -1;
                gojog.speed = jog_speed;
              }
              else if (jdoc["B"] == "plus")
              {
                gojog.axis = 2;
                gojog.position = jdoc["step"];
                gojog.dir = 1;
                gojog.speed = jog_speed;
              }
              else if (jdoc["B"] == "zero")
              {
                tool_correction[1] = current_position[1];
                current_position[1] = 0;
                jconf["tool_correctionX2"] = tool_correction[1];
              }
              else if (jdoc["B"] == "on")
              {
                startAC(20);
              }
              else if (jdoc["B"] == "off")
              {
                startAC(-20);
              }
            }

            if (jdoc.containsKey("Y"))
            {
              if (jdoc["Y"] == "?")
              {
                jdoc["Y"] = current_position[2];
                send = true;
              }
              else if (jdoc["Y"] == "minus")
              {
                gojog.axis = 4;
                gojog.position = jdoc["step"];
                gojog.dir = -1;
                gojog.speed = jog_speed;
              }
              else if (jdoc["Y"] == "plus")
              {
                gojog.axis = 4;
                gojog.position = jdoc["step"];
                gojog.dir = 1;
                gojog.speed = jog_speed;
              }
              else if (jdoc["Y"] == "zero")
              {
                tool_correction[2] = current_position[2];
                current_position[2] = 0;
                jconf["tool_correctionY"] = tool_correction[2];
              }
              else if (jdoc["Y"] == "on")
              {
                startAC(5);
              }
              else if (jdoc["Y"] == "off")
              {
                startAC(-30);
              }
            }

            if (send)
            {
              if (!getLimit(0b0000001))
                jdoc["End_A0"] = 1;
              if (!getLimit(0b0010000))
                jdoc["End_A"] = 1;
              if (!getLimit(0b0000010))
                jdoc["End_B0"] = 1;
              if (!getLimit(0b0100000))
                jdoc["End_B"] = 1;
              if (!getLimit(0b0000100))
                jdoc["End_Y0"] = 1;
              if (!getLimit(0b1000000))
                jdoc["End_Y"] = 1;
              if (getBit(BIT_IN_TISKI))
                jdoc["Tiski"] = 1;
              jdoc["process"] = process_mode;
              serializeJson(jdoc, output);
              client->text(output);
              //Serial.print("send:"); Serial.println(output);
            }
          }
        }
        */
      }
    }
  }
}

String statushtmlprocessor(const String &var)
{
  if (var == "SPEEDZ")
    return jconf["init"][0]["data"][0];
  if (var == "POSZ")
    return jconf["init"][1]["data"][0];
  if (var == "SPEED1")
    return jconf["init"][2]["data"][0];
  if (var == "SPEED2")
    return jconf["init"][3]["data"][0];
  if (var == "SPEED3")
    return jconf["init"][4]["data"][0];
  if (var == "POS1")
    return jconf["init"][6]["data"][0];
  if (var == "POS2")
    return jconf["init"][8]["data"][0];
  if (var == "POS3")
    return jconf["init"][10]["data"][0];

  if (var == "STATUS_TEXT")
    return status_text;

  return String();
}

void set_status(const char *text)
{
  status_text = String(text);
  net_send("status_text", status_text.c_str());
}

void net_setup()
{

  if (SPIFFS.begin(true /*formatOnFail*/))
  {
    Serial.println("mounted SPIFFS file system");

    size_t totalBytes = SPIFFS.totalBytes() / 1024;
    Serial.printf("SPIFFS file system Size: %zukB\n", totalBytes);
  }
  else
  {
    Serial.println("failed to mount SPIFS");
  }

  if (SPIFFS.exists(CONFIG_FILE))
  {
    // file exists, reading and loading
    Serial.println("reading config file");

    File configFile = SPIFFS.open(CONFIG_FILE, "r");
    if (configFile)
    {
      // Deserialize the JSON document
      DeserializationError error = deserializeJson(jconf, configFile);
      configFile.close();
      if (error)
        Serial.println(F("Failed to read CONFIG file!"));
    }
  }

  if (jconf["wifimode"] == (int)WIFI_STA) // STA = 1, AP = 2
  {
    Serial.print("Connect to: ");
    Serial.print(jconf["wifiname"].as<String>());
    WiFi.mode(WIFI_STA);
    WiFi.begin(jconf["wifiname"] | hostName, jconf["wifipass"] | "");
    int cnt = 10;
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
      cnt--;
      if (cnt == 0)
      {
        Serial.print("Not connect!");
        break;
      }
    }
  }

  Serial.println();

  Serial.print("STA IP Address: ");
  Serial.println(WiFi.localIP());

  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(jconf["wifissid"] | hostName);
    Serial.print("AP IP Address: ");
    Serial.println(WiFi.softAPIP());
  }

  ArduinoOTA.setHostname(jconf["hostname"] | hostName);
  ArduinoOTA.begin();

  // Send OTA events to the browser
  ArduinoOTA.onStart([]()
                     { events.send("Update Start", "ota"); });
  ArduinoOTA.onEnd([]()
                   { events.send("Update End", "ota"); });
  ArduinoOTA.onProgress(
      [](unsigned int progress, unsigned int total)
      {
        char p[32];
        sprintf(p, "Progress: %u%%\n", (progress / (total / 100)));
        events.send(p, "ota");
      });
  ArduinoOTA.onError(
      [](ota_error_t error)
      {
        if (error == OTA_AUTH_ERROR)
          events.send("Auth Failed", "ota");
        else if (error == OTA_BEGIN_ERROR)
          events.send("Begin Failed", "ota");
        else if (error == OTA_CONNECT_ERROR)
          events.send("Connect Failed", "ota");
        else if (error == OTA_RECEIVE_ERROR)
          events.send("Recieve Failed", "ota");
        else if (error == OTA_END_ERROR)
          events.send("End Failed", "ota");
      });

  Serial.print("ArduinoOTA hosthame: ");
  Serial.println(ArduinoOTA.getHostname());

  MDNS.addService("http", "tcp", 80);

  ws.onEvent(onWsEvent);

  server.addHandler(&ws);

  events.onConnect([](AsyncEventSourceClient *client)
                   { client->send("hello!", NULL, millis(), 1000); });
  server.addHandler(&events);

  server.addHandler(new SPIFFSEditor(SPIFFS, "", ""));

  server.on("/heap", HTTP_GET,
            [](AsyncWebServerRequest *request)
            {
              request->send(200, "text/plain", String(ESP.getFreeHeap()));
            });

  // Simple Firmware Update Form
  server.on("/update", HTTP_GET,
            [](AsyncWebServerRequest *request)
            {
              const char update_html_data[] PROGMEM = R"html(<!DOCTYPE html>
<html>
<head>
	<meta charset="utf-8" />
	<meta name="viewport" content="width=device-width, initial-scale=1" />
	<title>Firmware Update</title>
  </body>
</head>
<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>
<body>
</html>)html";
              request->send(200, "text/html", update_html_data);
            });

  server.on(
      "/update", HTTP_POST,
      [](AsyncWebServerRequest *request)
      {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
        response->addHeader("Connection", "close");
        request->send(response);
      },
      [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        if (!index)
        {
          Serial.print("Update ");
          // size_t content_len = request->contentLength();
          //  if filename includes spiffs, update the spiffs partition
          int cmd = (filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
          Serial.println((cmd == U_SPIFFS) ? "SPIFFS" : "FLASH");

          if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd))
          {
            {
              Update.printError(Serial);
            }
          }
        }

        if (!Update.hasError())
        {
          if (Update.write(data, len) != len)
          {
            Update.printError(Serial);
          }
        }

        if (final)
        {
          AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
          response->addHeader("Refresh", "20");
          response->addHeader("Location", "/");
          request->send(response);
          if (!Update.end(true))
          {
            Update.printError(Serial);
          }
          else
          {
            Serial.println("Update complete");
            Serial.flush();
          }

          shouldReboot = !Update.hasError();
        }
      });

  server.rewrite("/e", "/edit.htm");

  server.on("/erase", HTTP_GET,
            [](AsyncWebServerRequest *request)
            {
              erase_ee_data();
              request->redirect("/");
            });

  server.on("/read", HTTP_GET,
            [](AsyncWebServerRequest *request)
            {
              AsyncResponseStream *response = request->beginResponseStream("text/html");
              response->print(R"html(<!DOCTYPE html>
<html>
<head>
	<meta charset="utf-8" />
	<meta name="viewport" content="width=device-width, initial-scale=1" />
	<title>Woody</title>
  <style>
  body { font-family: monospace; }
  </style>
</head><body>)html");
              int i = 0;
              eeprom_block_t eeb;
              while (i < 128)
              {
                get_ee_data(&eeb, i);
                int *j = (int *)&eeb;
                while (j < (int *)((uint8_t *)&eeb + sizeof(eeprom_block_t)))
                {
                  response->printf("%08X,", *j);
                  j++;
                }
                response->print("<br>");
                i++;
              }

              response->print(R"html(</body></html>)html");
              request->send(response);
            });

  server.on("/", HTTP_GET,
            [](AsyncWebServerRequest *request)
            {
              AsyncResponseStream *response = request->beginResponseStream("text/html");
              response->print(R"html(<!DOCTYPE html>
<html>
<head>
	<meta charset="utf-8" />
	<meta name="viewport" content="width=device-width, initial-scale=1" />
	<title>Woody</title>
</head><body>)html");
              response->print(R"html(<table>
<thead>
<tr>
  <td> Дата </td>
  <td> Счетчик </td>
</tr> 
</thead><tbody>)html");

              int num = num_ee_block;
              int max = 32;
              int i = 0;
              eeprom_block_t eeb;
              extern eeprom_block_t ee;
              // response->printf("<tr><td align=\"right\">%2d.%02d.%4d</td><td>%d</td></tr>", ee.today.date.day, ee.today.date.month, ee.today.date.year2k + 2000, ee.today.counter);

              while (i++ < max)
              {
                get_ee_data(&eeb, num);
                //printf("%d: %d\n", num, eeb.total);

                if (eeb.total >= 0)
                {
                  response->printf("<tr><td align=\"right\">%d.%02d.%4d</td><td>%d</td></tr>", eeb.today.date.day, eeb.today.date.month, eeb.today.date.year2k + 2000, eeb.today.counter);
                }

                num--;
                if (num < 0)
                  num = 4096 / sizeof(eeprom_block_t) - 1;
              };

              response->printf("</tbody></table><p><b>Общий счетчик деталей:&nbsp;%d</b></p>", ee.total);
              //response->printf("<p><b>Общее время работы:&nbsp;%4d:%02d</b></p>", (ee.time / 60) / 60, (ee.time / 60) % 60);
              response->print(R"html(</body></html>)html");
              request->send(response);
            });

  server.serveStatic("/", SPIFFS, "/");

  server.onNotFound(
      [](AsyncWebServerRequest *request)
      {
        Serial.printf("NOT_FOUND: ");
        if (request->method() == HTTP_GET)
          Serial.printf("GET");
        else if (request->method() == HTTP_POST)
          Serial.printf("POST");
        else if (request->method() == HTTP_DELETE)
          Serial.printf("DELETE");
        else if (request->method() == HTTP_PUT)
          Serial.printf("PUT");
        else if (request->method() == HTTP_PATCH)
          Serial.printf("PATCH");
        else if (request->method() == HTTP_HEAD)
          Serial.printf("HEAD");
        else if (request->method() == HTTP_OPTIONS)
          Serial.printf("OPTIONS");
        else
          Serial.printf("UNKNOWN");
        Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

        if (request->contentLength())
        {
          Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
          Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
        }

        int headers = request->headers();
        int i;
        for (i = 0; i < headers; i++)
        {
          AsyncWebHeader *h = request->getHeader(i);
          Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
        }

        int params = request->params();
        for (i = 0; i < params; i++)
        {
          AsyncWebParameter *p = request->getParam(i);
          if (p->isFile())
          {
            Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
          }
          else if (p->isPost())
          {
            Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
          }
          else
          {
            Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
          }
        }
        request->send(404);
      });

  server.onFileUpload(
      [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        if (!index)
          Serial.printf("UploadStart: %s\n", filename.c_str());
        Serial.printf("%s", (const char *)data);
        if (final)
          Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index + len);
      });

  server.onRequestBody(
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
      {
        if (!index)
          Serial.printf("BodyStart: %u\n", total);
        Serial.printf("%s", (const char *)data);
        if (index + len == total)
          Serial.printf("BodyEnd: %u\n", total);
      });

  server.begin();

  st.begin();
  st.sync();
};

void net_process()
{
  ArduinoOTA.handle();

  if (shouldReboot == true)
  {
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
}

void net_send(const char *name, int32_t value)
{
  StaticJsonDocument<256> jdoc;
  char output[256];
  jdoc[name] = value;
  serializeJson(jdoc, output);
  ws.textAll(output);
}
void net_send(const char *name, const char *value)
{
  StaticJsonDocument<256> jdoc;
  char output[256];
  jdoc[name] = value;
  serializeJson(jdoc, output);
  ws.textAll(output);
}

bool get_setting(int pos, uint16_t *offset, uint16_t *values, uint16_t *count)
{
  JsonObject set = jconf["init"][pos];

  if (set.isNull())
  {
    return false;
  }

  *offset = set["offset"];
  JsonArray data = set["data"].as<JsonArray>();

  *count = 0;
  uint32_t v = 0;

  for (JsonVariant value : data)
  {
    v = value.as<int32_t>();
    values[*count] = v & 0xFFFF;
    (*count)++;
    values[*count] = v >> 16;
    (*count)++;
  }
  if (*count == 0)
    return false;

  return true;
}
