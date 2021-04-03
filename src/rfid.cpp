#include <rfid.h>
#include "syslog.h"


MFRC522 mfrc522 = MFRC522();

bool ICACHE_FLASH_ATTR EvseWiFiRfid::begin(int rfidss, bool usePN532, int rfidgain, NtpClient* ntp, bool debug) {
  this->debug = debug;
  this->ntpClient = ntp;
  mfrc522.PCD_SetAntennaGain(rfidgain);
  delay(50);
  mfrc522.PCD_Init(rfidss, 0);
  delay(50);
  sysLog.debug("RFID SS_PIN: " + String(rfidss) + " and Gain Factor: " + String(rfidgain), SysLog::RFID);
  delay(50);
  printReaderDetails();
  return true;
}

void ICACHE_FLASH_ATTR EvseWiFiRfid::printReaderDetails() {
  // Get the MFRC522 software version 
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  String ver = "unknown";
  switch(v) {
    case 0x91: ver = "v1.0"; break;
    case 0x92: ver = "v2.0"; break;
    case 0x88: ver = "clone"; break;
  }
  sysLog.debug("MFRC522 Version: 0x" + String(v,HEX) + ver, SysLog::RFID);
  if ((v == 0x00) || (v == 0xFF)) {
    sysLog.error(F("Communication failure, check if MFRC522 properly connected"), SysLog::RFID);
  }
}

scanResult ICACHE_FLASH_ATTR EvseWiFiRfid::readPicc() {
  scanResult res;
  //RC522
    if (! mfrc522.PICC_IsNewCardPresent()) {
      delay(50);
      res.read = false;
      this->cooldown = millis() + 50;
      return res;
    }
    if (! mfrc522.PICC_ReadCardSerial()) {
      delay(50);
      res.read = false;
      this->cooldown = millis() + 50;
      return res;
    }
    res.read = true;
    sysLog.debug(F("Card detected to read!"), SysLog::RFID);
    mfrc522.PICC_HaltA();
    this->cooldown = millis() + 3000;

    String uid = "";
    for (int i = 0; i < mfrc522.uid.size; ++i) {
      uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    res.uid = uid;
    sysLog.debug("PICC's UID: " + uid, SysLog::RFID);

    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    String type = mfrc522.PICC_GetTypeName(piccType);
    res.type = type;

    //SPIFFS.begin();
    int AccType = 0;
    String filename = "/P/";
    filename += res.uid;
    File rfidFile = SPIFFS.open(filename, "r");
    #ifdef ESP8266
    if (rfidFile)  // Known PICC
    #else
    if (SPIFFS.exists(filename)) 
    #endif
    {
      res.known = true;
      String jsonString = "";
      while (rfidFile.available()) {
        jsonString += char(rfidFile.read());
      }
      StaticJsonDocument<200> jsonDoc;
      DeserializationError error = deserializeJson(jsonDoc, jsonString);
      if (!error) {
        res.user = strdup(jsonDoc["user"]);
        AccType = jsonDoc["acctype"];
        sysLog.debug(F(" = known PICC"), SysLog::RFID);

        String usrTmp = res.user;
        if (res.user == "undefined") {
          String usrTmp = res.uid;
        }

        if ((AccType == 1 || AccType == 99) &&
            ntpClient->getUtcTimeNow() < jsonDoc["validuntil"]) {
          res.valid = true;
        }
        sysLog.debug("User Name: " + res.uid + " has permission: " + res.valid ? "true" : "false", SysLog::RFID);
      }
      else {
        sysLog.warning(F("Failed to parse User Data"), SysLog::RFID);
      }
    }
    else { // Unknown PICC
      sysLog.warning(F(" = unknown PICC"), SysLog::RFID);
    }
    rfidFile.close();
    //SPIFFS.end();

 return res;
}

DynamicJsonDocument ICACHE_FLASH_ATTR EvseWiFiRfid::getUserList(int page) {
  sysLog.info("getUserlist - Page: " + String(page), SysLog::RFID);
  DynamicJsonDocument jsonDoc(3000);
  jsonDoc["command"] = "userlist";
  jsonDoc["page"] = page;
  JsonArray users = jsonDoc.createNestedArray("list");
  //SPIFFS.begin();
  #ifdef ESP8266
  Dir dir = SPIFFS.openDir("/P/");
  #else
  File dir = SPIFFS.open("/P");
  #endif
  int first = (page - 1) * 15;
  int last = page * 15;
  int i = 0;
  #ifdef ESP8266
  while (dir.next()) {
  #else
  File file = dir.openNextFile();
  while (file) {
  #endif
    if (i >= first && i < last) {
      JsonObject item = users.createNestedObject();
      #ifdef ESP8266
      String uid = dir.fileName();
      File f = SPIFFS.open(dir.fileName(), "r");
      #else
      String uid = file.name();
      File f = SPIFFS.open(file.name(), "r");
      #endif
      uid.remove(0, 3);
      item["uid"] = uid;
      size_t size = f.size();
      std::unique_ptr<char[]> buf(new char[size]);
      f.readBytes(buf.get(), size);
      StaticJsonDocument<200> jsonDoc2;
      DeserializationError error = deserializeJson(jsonDoc2, buf.get());
      if (!error) {
        String username = jsonDoc2["user"];
        int AccType = jsonDoc2["acctype"];
        unsigned long validuntil = jsonDoc2["validuntil"];
        item["username"] = username;
        item["acctype"] = AccType;
        item["validuntil"] = validuntil;
      }
    }
    i++;
    #ifdef ESP32
    file = dir.openNextFile();
    #endif
  }
  #ifndef ESP8266
  file.close();
  #endif
  //SPIFFS.end();
  serializeJson(jsonDoc, Serial);
  float pages = i / 15.0;
  jsonDoc["haspages"] = ceil(pages);
  return jsonDoc;
}

bool ICACHE_FLASH_ATTR EvseWiFiRfid::performSelfTest() {
  return mfrc522.PCD_PerformSelfTest();
}

bool ICACHE_FLASH_ATTR EvseWiFiRfid::reset() {
  mfrc522.PCD_Init();
  return true;
}