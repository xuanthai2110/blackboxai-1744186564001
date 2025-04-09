#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ModbusMaster.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include "SPIFFS.h"

// Create three Modbus instances for PZEM-016
ModbusMaster pzem1;
ModbusMaster pzem2;
ModbusMaster pzem3;

// Create AsyncWebServer object
AsyncWebServer server(80);

// PZEM-016 register addresses
#define PZEM_VOLTAGE     0x0000
#define PZEM_CURRENT     0x0001
#define PZEM_POWER       0x0002
#define PZEM_ENERGY      0x0003
#define PZEM_FREQUENCY   0x0004
#define PZEM_PF          0x0005

float readPZEMValue(ModbusMaster &pzem, uint16_t reg) {
  uint8_t result = pzem.readInputRegisters(reg, 1);
  if (result == pzem.ku8MBSuccess) {
    return pzem.getResponseBuffer(0);
  }
  return 0;
}

void setup() {
  Serial.begin(115200);
  
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Initialize Modbus communication
  Serial1.begin(9600, SERIAL_8N1, 16, 17); // UART2 on GPIO 16 (RX), 17 (TX)
  pzem1.begin(1, Serial1);
  pzem2.begin(2, Serial1);
  pzem3.begin(3, Serial1);

  // WiFi Manager
  WiFiManager wifiManager;
  wifiManager.autoConnect("ESP32-PZEM");
  
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // Serve static files
  server.serveStatic("/", SPIFFS, "/");

  // Route for JSON data
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(1024);
    
    // Read and add PZEM-016 data
    for(int i=1; i<=3; i++) {
      ModbusMaster* pzem = i==1 ? &pzem1 : (i==2 ? &pzem2 : &pzem3);
      char key[10];
      sprintf(key, "pzem%d", i);
      
      doc[key]["voltage"] = readPZEMValue(*pzem, PZEM_VOLTAGE) / 10.0;
      doc[key]["current"] = readPZEMValue(*pzem, PZEM_CURRENT) / 1000.0;
      doc[key]["power"] = readPZEMValue(*pzem, PZEM_POWER) / 10.0;
      doc[key]["energy"] = readPZEMValue(*pzem, PZEM_ENERGY) / 1000.0;
      doc[key]["frequency"] = readPZEMValue(*pzem, PZEM_FREQUENCY) / 10.0;
      doc[key]["pf"] = readPZEMValue(*pzem, PZEM_PF) / 100.0;
    }
    
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
  });

  // Start server
  server.begin();
}

void loop() {
  // Nothing to do here - async server handles requests
}
