#include "WiFi.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include <ArduinoJson.h>
#include <HTTPClient.h>


/* Serial Baud Rate */
#define SERIAL_BAUD       9600
/* Delay paramter for connection. */
#define WIFI_DELAY        500
/* Max SSID octets. */
#define MAX_SSID_LEN      32
/* Wait this much until device gets IP. */
#define MAX_CONNECT_TIME  30000
#define MAX_URL_LENGTH  255
#define JSON_DOC_SIZE  4096

File file;
File logger;
int readError = 0;
const char* configFile = "/tasmota.json";
const char* logFile = "/tasmotaflash.log";
char specials[] = "$&+,/:;=?@ <>#%{}|~[]`"; ///* String containing chars you want encoded */
const char* urlPrefix = "/cm?cmnd=Backlog%20";
JsonArray config;
StaticJsonDocument<JSON_DOC_SIZE> doc;
String tasmotaSSID;
String macAddr;
void setup()
{
  Serial.begin(115200);
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  delay(WIFI_DELAY);
  if (!SD_MMC.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD_MMC card attached");
    return;
  }
  logger = SD_MMC.open(logFile, FILE_WRITE);
  Serial.println("Tasmota Auto Configurator Starting...");
  if (logger)
    Serial.println("Opened log file for writing");
  file = SD_MMC.open(configFile);
  while (file.available()) {
    Serial.println("Tasmota Auto Configurator Starting...");
    Serial.print("Reading JSON File.");
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
      Serial.print("Error Reading JSON:");
      Serial.print(error.c_str());
      readError = 1;
    }
    Serial.println("OK");
    logger.println("Parsed JSON Successfully");
    tasmotaSSID.reserve(MAX_SSID_LEN);
    macAddr.reserve(MAX_SSID_LEN);
  }
  if (!readError)
  {
    if (WiFi.status() != WL_CONNECTED) {
      scanAndConfigure();
    }
    delay(10000);
  }
  logger.close();

}
void loop()
{
}

void scanAndConfigure() {
  Serial.print("Scan start ... ");
  int n = WiFi.scanNetworks();

  Serial.println("Scan complete!");
  if (n == 0) {
    Serial.println("No networks available.");
  } else {
    Serial.print(n);
    Serial.println(" networks discovered.");
    int indices[n];
    for (int i = 0; i < n; i++) {
      indices[i] = i;
    }
    for (int i = 0; i < n; ++i) {
      if (WiFi.encryptionType(indices[i]) == 0) {
        Serial.print(WiFi.SSID(indices[i]));
        Serial.print("(");
        Serial.print(WiFi.RSSI(indices[i]));
        Serial.print(" db)");
        Serial.print(WiFi.encryptionType(indices[i]));
        Serial.println();
        tasmotaSSID = WiFi.SSID(indices[i]);
        /* Global ssid param need to be filled to connect. */
        if (tasmotaSSID.substring(0, 7) == "tasmota") {
          Serial.print("Connecting to Tasmota Device with SSID " + tasmotaSSID);
          WiFi.begin(tasmotaSSID.c_str());
          unsigned short try_cnt = 0;
          /* Wait until WiFi connection but do not exceed MAX_CONNECT_TIME */
          while (WiFi.status() != WL_CONNECTED && try_cnt < MAX_CONNECT_TIME / WIFI_DELAY) {
            delay(WIFI_DELAY);
            Serial.print(".");
            try_cnt++;
          }
          if (WiFi.status() == WL_CONNECTED) {
            Serial.println("");
            Serial.println("Connection Successful!");
            Serial.print("Your device IP address is ");
            Serial.println(WiFi.localIP());
            Serial.println("Gateway Ip:" + WiFi.gatewayIP().toString());
            macAddr = WiFi.BSSIDstr();
            macAddr.toLowerCase();
            Serial.println("Tasmota MAC Address " + macAddr);
            JsonObject root = doc["config"];
            if (!root.isNull())
            {
              logger.println("Universal Config Mode");
              Serial.println("Universal Config Mode");
            }
            else
            {
              root = doc[macAddr];
              if (!root.isNull())
                Serial.println("Found Config Entry for this device.");
              else
                Serial.println("Could not find config for this device.Skipping.");
            }
            if (!root.isNull())
            {
              char commandURL[MAX_URL_LENGTH];
              char commandURLEncoded[MAX_URL_LENGTH];
              char url[MAX_URL_LENGTH];
              strcpy(url, ("http://" + WiFi.gatewayIP().toString() + urlPrefix).c_str());
              for (JsonPair kv : root) {
                strcat(commandURL, kv.key().c_str());
                strcat(commandURL, " ");
                strcat(commandURL, kv.value().as<char*>());
                strcat(commandURL, ";");
              }
              urlencode(commandURLEncoded, commandURL);
              strcat(url, commandURLEncoded);
              Serial.println("Attempting to send config params to device");
              HTTPClient http;
              http.begin(url);     //Specify request destination

              int httpCode = http.GET();            //Send the request
              String payload = http.getString();    //Get the response payload
              char responseCode[5];
              sprintf(responseCode, "%d", httpCode);

              if (httpCode == 200) {
                Serial.println("Device configured successfully");
                logger.println(macAddr + ":OK");
              }
              else
              {
                Serial.println("Couldnt configure device.");
                logger.println(macAddr + ":ERROR");
              }
              http.end();  //Close connection
            }
            WiFi.disconnect();

          } else {
            Serial.println("Connection FAILED");
          }
        } else {
          Serial.println("No open networks available. :-(");
        }
      }
    }
  }
}


static char hex_digit(char c)
{ return "0123456789ABCDEF"[c & 0x0F];
}

char* urlencode(char* dst, char* src)
{

  char *d = dst;
  char c;
  while (c = *src++)
  { if (strchr(specials, c))
    { *d++ = '%';
      *d++ = hex_digit(c >> 4);
      *d++ = hex_digit(c);
    }
    else *d++ = c;
  }
  return dst;
}
