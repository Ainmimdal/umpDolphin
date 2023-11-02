#include <RTClib.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "fs.h"
#include "webpages.h"
#include "driver/rtc_io.h"
#include "MS5837.h"


#define SCL 22
#define SDA 23
// Chip select for the microSD card
#define SD_CARD_SELECT 33

#define swR 34

#define FIRMWARE_VERSION "v0.0.1"

const String default_ssid = "somessid";
const String default_wifipassword = "mypassword";
const String default_httpuser = "admin";
const String default_httppassword = "admin";
const int default_webserverporthttp = 80;

// configuration structure
struct Config {
  String ssid;            // wifi ssid
  String wifipassword;    // wifi password
  String httpuser;        // username to access web admin
  String httppassword;    // password to access web admin
  int webserverporthttp;  // http port number for web admin
};

// slots for the event log file
File logfile;
RTC_DATA_ATTR boolean recordingEvents = false;
RTC_DATA_ATTR char logFileName[20];

// variables
Config config;              // configuration
bool shouldReboot = false;  // schedule a reboot
AsyncWebServer* server;     // initialise webserver
String timestamp;
// function defaults
String listFiles(bool ishtml = false);
RTC_PCF8523 rtc;
MS5837 sensor;

void setup() {
  Serial.begin(115200);

  Serial.print("Firmware: ");
  Serial.println(FIRMWARE_VERSION);

  Serial.println("Booting ...");
  //bluerobotics bar30
  Wire.begin();

  sensor.setModel(MS5837::MS5837_30BA);
  sensor.init();

  sensor.setFluidDensity(997);  // kg/m^3 (997 freshwater, 1029 for seawater)
                                //
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  pinMode(swR,INPUT_PULLUP);
  // Initialize the SD card
  pinMode(SD_CARD_SELECT, OUTPUT);
  if (!SD.begin(SD_CARD_SELECT)) {
    Serial.println("no sd");
    //while (1) delay (1000);
  }

  // create an event log .csv file using the counter startup date
  boolean newfile = false;
  if (true) {
    eventsFileName(logFileName, getCurrentTime());
    if (!SD.exists(logFileName)) {
      newfile = true;
    }
    if (newfile) {
      Serial.print("\nCreating file");
    } else {
      Serial.print("\nOpening file");
    }
    Serial.println(logFileName);
  }

  logfile = SD.open(logFileName, "a+");
  if (!logfile) {
    Serial.println("error opening log, check sd");
    while (1) delay(1000);
  }
  recordingEvents = true;

  if (newfile) {
    logfile.println("Timestamp, Pressure, Temperature, Depth");
    logfile.flush();
  }

  // Serial.println("Mounting SPIFFS ...");
  // if (!SPIFFS.begin(true)) {
  //   // if you have not used SPIFFS before on a ESP32, it will show this error.
  //   // after a reboot SPIFFS will be configured and will happily work.
  //   Serial.println("ERROR: Cannot mount SPIFFS, Rebooting");
  //   rebootESP("ERROR: Cannot mount SPIFFS, Rebooting");
  // }

  // // Serial.print("SPIFFS Free: "); Serial.println(humanReadableSize((SD.totalBytes() - SPIFFS.usedBytes())));
  // // Serial.print("SPIFFS Used: "); Serial.println(humanReadableSize(SPIFFS.usedBytes()));
  // // Serial.print("SPIFFS Total: "); Serial.println(humanReadableSize(SPIFFS.totalBytes()));

  //  Serial.print("SPIFFS Free: "); Serial.println(humanReadableSize((SD.totalBytes() - SPIFFS.usedBytes())));
  // Serial.print("SPIFFS Used: "); Serial.println(humanReadableSize(SPIFFS.usedBytes()));
  // Serial.print("SPIFFS Total: "); Serial.println(humanReadableSize(SPIFFS.totalBytes()));

  // Serial.println(listFiles());

  Serial.println("Loading Configuration ...");

  config.ssid = default_ssid;
  config.wifipassword = default_wifipassword;
  config.httpuser = default_httpuser;
  config.httppassword = default_httppassword;
  config.webserverporthttp = default_webserverporthttp;

  Serial.print("\nConnecting to Wifi: ");
  WiFi.softAP("ump", "12345678");
  Serial.println(WiFi.softAPIP());
  // WiFi.begin(config.ssid.c_str(), config.wifipassword.c_str());
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }

  Serial.println("\n\nNetwork Configuration:");
  Serial.println("----------------------");
  Serial.print("         SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("  Wifi Status: ");
  Serial.println(WiFi.status());
  Serial.print("Wifi Strength: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.print("          MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("           IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("       Subnet: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("      Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("        DNS 1: ");
  Serial.println(WiFi.dnsIP(0));
  Serial.print("        DNS 2: ");
  Serial.println(WiFi.dnsIP(1));
  Serial.print("        DNS 3: ");
  Serial.println(WiFi.dnsIP(2));
  Serial.println();

  // configure web server
  Serial.println("Configuring Webserver ...");
  server = new AsyncWebServer(config.webserverporthttp);
  configureWebServer();

  // startup web server
  Serial.println("Starting Webserver ...");
  server->begin();
}

void loop() {
  // reboot if we've told it to reboot
  if (shouldReboot) {
    rebootESP("Web Admin Initiated Reboot");
  }

  DateTime now = getCurrentTime();
  if(digitalRead(swR)==1){
    recordingEvents=false;
  }
  String dataString;
  if (recordingEvents) {
    dataString += getTimestampString(now);
    dataString += ",";
    dataString += sensor.pressure();  //Celcius
    dataString += ",";
    dataString += sensor.temperature();  //%
    dataString += ",";
    dataString += sensor.depth();  //%
    logfile.println(dataString);
    logfile.flush();
  }


  Serial.println(dataString);  //just for debuging
  delay(2000);
}

void rebootESP(String message) {
  Serial.print("Rebooting ESP32: ");
  Serial.println(message);
  ESP.restart();
}

// list all of the files, if ishtml=true, return html rather than simple text
String listFiles(bool ishtml) {
  String returnText = "";
  Serial.println("Listing files stored on SPIFFS");
  //File root = SPIFFS.open("/");
  File root = SD.open("/");
  File foundfile = root.openNextFile();
  if (ishtml) {
    returnText += "<table><tr><th align='left'>Name</th><th align='left'>Size</th><th></th><th></th></tr>";
  }
  while (foundfile) {
    if (ishtml) {
      returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td>" + humanReadableSize(foundfile.size()) + "</td>";
      returnText += "<td><button onclick=\"downloadDeleteButton(\'" + String(foundfile.name()) + "\', \'download\')\">Download</button>";
      returnText += "<td><button onclick=\"downloadDeleteButton(\'" + String(foundfile.name()) + "\', \'delete\')\">Delete</button></tr>";
    } else {
      returnText += "File: " + String(foundfile.name()) + " Size: " + humanReadableSize(foundfile.size()) + "\n";
    }
    foundfile = root.openNextFile();
  }
  if (ishtml) {
    returnText += "</table>";
  }
  root.close();
  foundfile.close();
  return returnText;
}

// Make size of files human readable
// source: https://github.com/CelliesProjects/minimalUploadAuthESP32
String humanReadableSize(const size_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
  else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
  else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

//
// Get the current time
//
DateTime getCurrentTime() {
  return rtc.now();
}

//
// Get a time string in format mm-dd-yy hh:mm:ss, which seems to make
// most spreadsheets happy. RTCs vary and may or may not support 4 digit dates,
// so we always just use last 2 for consistency.
//
String getTimestampString(DateTime aTime) {
  int thisYear = aTime.year();
  if (thisYear > 2000) {
    thisYear = thisYear - 2000;
  }
  char buffer[20];
  sprintf(buffer, "%02d-%02d-%02d %02d:%02d:%02d", aTime.month(), aTime.day(), thisYear, aTime.hour(), aTime.minute(), aTime.second());
  return String(buffer);
}

//
// Compute a FAT filename for the events log
//
void eventsFileName(char* buffer, DateTime aTime) {
  int aYear = aTime.year();
  if (aYear > 2000) {
    aYear = aYear - 2000;
  }
  sprintf(buffer, "/C%02d%02d%02d.csv", aYear, aTime.month(), aTime.day());
}
