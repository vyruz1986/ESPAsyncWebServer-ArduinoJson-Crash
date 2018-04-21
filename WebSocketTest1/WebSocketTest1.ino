#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <Hash.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// SKETCH BEGIN
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
   if (type == WS_EVT_CONNECT) {
      Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
      client->printf("Hello Client %u :)", client->id());
      client->ping();
   }
   else if (type == WS_EVT_DISCONNECT) {
      Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
   }
   else if (type == WS_EVT_ERROR) {
      Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
   }
   else if (type == WS_EVT_PONG) {
      Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char*)data : "");
   }
   else if (type == WS_EVT_DATA) {
      AwsFrameInfo * info = (AwsFrameInfo*)arg;
      String msg = "";
      if (info->final && info->index == 0 && info->len == len) {
         //the whole message is in a single frame and we got all of it's data
         Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

         if (info->opcode == WS_TEXT) {
            for (size_t i = 0; i < info->len; i++) {
               msg += (char)data[i];
            }
         }
         else {
            char buff[3];
            for (size_t i = 0; i < info->len; i++) {
               sprintf(buff, "%02x ", (uint8_t)data[i]);
               msg += buff;
            }
         }
         Serial.printf("%s\n", msg.c_str());

         if (info->opcode == WS_TEXT)
            client->text("I got your text message");
         else
            client->binary("I got your binary message");
      }
      else {
         //message is comprised of multiple frames or the frame is split into multiple packets
         if (info->index == 0) {
            if (info->num == 0)
               Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
            Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
         }

         Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

         if (info->opcode == WS_TEXT) {
            for (size_t i = 0; i < info->len; i++) {
               msg += (char)data[i];
            }
         }
         else {
            char buff[3];
            for (size_t i = 0; i < info->len; i++) {
               sprintf(buff, "%02x ", (uint8_t)data[i]);
               msg += buff;
            }
         }
         Serial.printf("%s\n", msg.c_str());

         if ((info->index + len) == info->len) {
            Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
            if (info->final) {
               Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
               if (info->message_opcode == WS_TEXT)
                  client->text("I got your text message");
               else
                  client->binary("I got your binary message");
            }
         }
      }
   }
}

unsigned long lLastSerialOutput = 0;

//WiFiMulti wm;
IPAddress IPGateway(192, 168, 20, 1);
IPAddress IPNetwork(192, 168, 20, 0);
IPAddress IPSubnet(255, 255, 255, 0);

void setup() {
   Serial.begin(115200);
   Serial.setDebugOutput(true);

   //Setup AP
   WiFi.config(IPGateway, IPGateway, IPSubnet);
   if (!WiFi.softAPConfig(IPGateway, IPGateway, IPSubnet)) {
      Serial.printf("[WiFi]: AP Config failed!\r\n");
   }

   WiFi.mode(WIFI_MODE_AP);
   String strAPName = "TestAP";
   String strAPPass = "TestAPPass";
   if (!WiFi.softAP(strAPName.c_str(), strAPPass.c_str()))
   {
      Serial.printf("Error initializing softAP!\r\n");
   }
   else
   {
      Serial.printf("Wifi started successfully, AP name: %s!\r\n", strAPName.c_str());
   }
   WiFi.onEvent(WiFiEvent);

   Serial.printf("Listening on %s\r\n", WiFi.localIP().toString().c_str());

   MDNS.addService("http", "tcp", 80);
   MDNS.begin("WebSocketTest");

   ws.onEvent(onWsEvent);
   server.addHandler(&ws);
   server.begin();
}

void loop() {
   if ((millis() - lLastSerialOutput) > 200)
   {
      Serial.printf("Free heap: %d, uptime: %lu\r\n", system_get_free_heap_size(), millis());

      //Using this string does NOT cause a crash!
      //String JsonString = "{\"RaceData\":{\"id\":99,\"startTime\":66165,\"endTime\":89165,\"elapsedTime\":67165,\"totalCrossingTime\":1234,\"raceState\":1,\"dogData\":[{\"dogNumber\":0,\"timing\":[{\"time\":5332,\"crossingTime\":1498},{\"time\":3787,\"crossingTime\":150},{\"time\":4114,\"crossingTime\":2763},{\"time\":6718,\"crossingTime\":3242}],\"fault\":false,\"running\":false},{\"dogNumber\":1,\"timing\":[{\"time\":5612,\"crossingTime\":3253},{\"time\":4663,\"crossingTime\":1045},{\"time\":3843,\"crossingTime\":3365},{\"time\":6058,\"crossingTime\":2640}],\"fault\":false,\"running\":false},{\"dogNumber\":2,\"timing\":[{\"time\":4367,\"crossingTime\":762},{\"time\":3426,\"crossingTime\":3186},{\"time\":5947,\"crossingTime\":1625},{\"time\":5692,\"crossingTime\":3794}],\"fault\":false,\"running\":false},{\"dogNumber\":3,\"timing\":[{\"time\":4919,\"crossingTime\":4253},{\"time\":4331,\"crossingTime\":520},{\"time\":4811,\"crossingTime\":2550},{\"time\":3385,\"crossingTime\":3515}],\"fault\":false,\"running\":false}]}}";

      //But using the function below (which uses ArduinoJSON) does cause the crash
      String JsonString;
      GetRaceDataJsonString(JsonString);
      
      ws.textAll(JsonString);
      lLastSerialOutput = millis();
   }
}


void WiFiEvent(WiFiEvent_t event) {
   switch (event) {
   case SYSTEM_EVENT_AP_START:
      Serial.printf("AP Started");
      if (!WiFi.softAPConfig(IPGateway, IPGateway, IPSubnet)) {
         Serial.printf("[WiFi]: AP Config failed!");
      }
      break;
   case SYSTEM_EVENT_AP_STOP:
      Serial.printf("AP Stopped");
      break;

   default:
      break;
   }
}

boolean GetRaceDataJsonString(String &strJsonString)
{
   const size_t bufferSize = 5 * JSON_ARRAY_SIZE(4) + JSON_OBJECT_SIZE(1) + 16 * JSON_OBJECT_SIZE(2) + 4 * JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(7);
   DynamicJsonBuffer JsonBuffer(bufferSize);
   JsonObject& JsonRoot = JsonBuffer.createObject();

   JsonObject& JsonRaceData = JsonRoot.createNestedObject("RaceData");
   JsonRaceData["id"] = 99;
   JsonRaceData["startTime"] = (micros() - 24000000) / 1000;
   JsonRaceData["endTime"] = (micros() - 1000000) / 1000;
   JsonRaceData["elapsedTime"] = (micros() - 23000000) / 1000;
   JsonRaceData["totalCrossingTime"] = 1234;
   JsonRaceData["raceState"] = 1;
   JsonArray& JsonDogDataArray = JsonRaceData.createNestedArray("dogData");
   for (uint8_t i = 0; i < 4; i++)
   {
      JsonObject& JsonDogData = JsonDogDataArray.createNestedObject();
      JsonDogData["dogNumber"] = i;
      JsonArray& JsonDogDataTimingArray = JsonDogData.createNestedArray("timing");
      for (uint8_t i2 = 0; i2 < 4; i2++)
      {
         JsonObject& DogTiming = JsonDogDataTimingArray.createNestedObject();
         DogTiming["time"] = random(3000, 7000);
         DogTiming["crossingTime"] = random(0, 5000);
      }
      JsonDogData["fault"] = random(0, 1);
      JsonDogData["running"] = false;
   }

   JsonRoot.printTo(strJsonString);

   return true;
}