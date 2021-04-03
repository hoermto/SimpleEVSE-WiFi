#include "config.h"
#include "templates.h"
#include "syslog.h"

EvseWiFiConfig::EvseWiFiConfig(const String cfgFile)
    : configFileName(cfgFile) {
#ifdef ESP32
    modbusConfig.enabled = true;
    modbusConfig.rxPin = 22;
    modbusConfig.txPin = 21;
    evseConfig[0].useModbus = true;
    evseConfig[0].serialRxPin = 22; // ignored with using modbus
    evseConfig[0].serialTxPin = 21; // ignored with using modbus
    evseConfig[0].alwaysactive = false;
#endif
    if (configFileName == "") {
        configFileName = "/config.json";
    }
};

// deletes configuration file and restores
// configuration as initial start
bool ICACHE_FLASH_ATTR EvseWiFiConfig::factoryReset() {
    SPIFFS.remove(configFileName);
    return true;
}

bool ICACHE_FLASH_ATTR EvseWiFiConfig::loadConfig(String givenConfig) {
    String jsonString;
    bool loadDefault = false;
    configLoaded = false;
    pre_0_4_Config = false;
    
    if (givenConfig != "") {
        sysLog.info(F("loadConfig: given config string ..."), SysLog::CONFIG);
        jsonString = givenConfig;
    }
    else {
        //SPIFFS.begin();
        sysLog.info(F("loadConfig: no config string given -> check config file"), SysLog::CONFIG);
        File configFile = SPIFFS.open(configFileName, "r");
        #ifdef ESP8266
        if (!configFile) {
            sysLog.error(F("loading config file failed"), SysLog::CONFIG);
            return false;
        }
        #else
        if (!configFile.available()) {
            configFile.close();
            sysLog.warning(String(F("loading config file failed - rebuild factory settings: ")) + configFileName, SysLog::CONFIG);
            loadDefault = true;
        }
        #endif
        if (loadDefault) {
            jsonString = SRC_CONFIG_TEMPLATE;
        }
        else {
            while (configFile.available()) {
                jsonString += char(configFile.read());
            }
        }
        //SPIFFS.end();
    }

    DynamicJsonDocument jsonDoc(2000);
    DeserializationError error = deserializeJson(jsonDoc, jsonString);
    if (error) {
        sysLog.error("parsing config file failed: " + String(error.c_str()), SysLog::CONFIG);
        return false;
    }
    
    if (!jsonDoc.containsKey("configversion")) {
        pre_0_4_Config = true;
        sysLog.info(F("pre 0.4 config file found..."), SysLog::CONFIG);
    }
    else {
        sysLog.info(F("actual config file found"), SysLog::CONFIG);
    }

    // wifiConfig
    wifiConfig.bssid = strdup(jsonDoc["wifi"]["bssid"]);
    wifiConfig.ssid = strdup(jsonDoc["wifi"]["ssid"]);
    wifiConfig.wmode = jsonDoc["wifi"]["wmode"];
    wifiConfig.pswd = strdup(jsonDoc["wifi"]["pswd"]);
    wifiConfig.staticip = jsonDoc["wifi"]["staticip"];
    wifiConfig.ip = strdup(jsonDoc["wifi"]["ip"]);
    wifiConfig.subnet = strdup(jsonDoc["wifi"]["subnet"]);
    wifiConfig.gateway = strdup(jsonDoc["wifi"]["gateway"]);
    wifiConfig.dns = strdup(jsonDoc["wifi"]["dns"]);
    sysLog.info(F("WIFI loaded"), SysLog::CONFIG);

    // meterConfig
    meterConfig[0].usemeter = jsonDoc["meter"][0]["usemeter"];
    meterConfig[0].metertype = strdup(jsonDoc["meter"][0]["metertype"]);
    meterConfig[0].price = jsonDoc["meter"][0]["price"];
    meterConfig[0].intpin = jsonDoc["meter"][0]["intpin"];
    meterConfig[0].kwhimp = jsonDoc["meter"][0]["kwhimp"];
    meterConfig[0].implen = jsonDoc["meter"][0]["implen"];
    meterConfig[0].meterphase = jsonDoc["meter"][0]["meterphase"];
    meterConfig[0].factor = jsonDoc["meter"][0]["factor"];
    sysLog.info(F("Meter loaded"), SysLog::CONFIG);

    // rfidConfig
    rfidConfig.userfid = jsonDoc["rfid"]["userfid"];
    rfidConfig.sspin = jsonDoc["rfid"]["sspin"];
    rfidConfig.rfidgain = jsonDoc["rfid"]["rfidgain"];
    sysLog.info(F("RFID loaded"), SysLog::CONFIG);


    // ntpConfig
    ntpConfig.timezone = jsonDoc["ntp"]["timezone"];
    ntpConfig.ntpip = strdup(jsonDoc["ntp"]["ntpip"]);
    sysLog.info(F("NPD loaded"), SysLog::CONFIG);
    if (jsonDoc["ntp"].containsKey("dst")) {
        ntpConfig.dst = jsonDoc["ntp"]["dst"];
    }
    else {
        ntpConfig.dst = false;
    }

    // buttonConfig
    buttonConfig[0].usebutton = jsonDoc["button"][0]["usebutton"];
    buttonConfig[0].buttonpin = jsonDoc["button"][0]["buttonpin"];
    sysLog.info(F("BUTTON loaded"), SysLog::CONFIG);


    // systemConfig
    systemConfig.hostnm = strdup(jsonDoc["system"]["hostnm"]);
    systemConfig.adminpwd = strdup(jsonDoc["system"]["adminpwd"]);
    systemConfig.wsauth = jsonDoc["system"]["wsauth"];
    systemConfig.debug = jsonDoc["system"]["debug"];
    if( systemConfig.debug ) {
        sysLog.setLogLevel(SysLog::DEBUG);
    } else {
        sysLog.setLogLevel(SysLog::INFO);
    }
    systemConfig.maxinstall = jsonDoc["system"]["maxinstall"];
    systemConfig.evsecount = jsonDoc["system"]["evsecount"];
    systemConfig.configversion = jsonDoc["configversion"];
    if (jsonDoc["system"].containsKey("logging")) {
        systemConfig.logging = jsonDoc["system"]["logging"];
    }
    else {
        systemConfig.logging = true;
    }
    if (jsonDoc["system"].containsKey("api")) {
        systemConfig.api = jsonDoc["system"]["api"];
    }
    else {
        systemConfig.api = true;
    }
    sysLog.info(F("SYSTEM loaded"), SysLog::CONFIG);


#ifdef ESP32
    // modbus config
    if (jsonDoc.containsKey("modbus"))
    {
        modbusConfig.enabled = jsonDoc["modbus"]["enabled"];
        modbusConfig.rxPin = jsonDoc["modbus"]["rxpin"];
        modbusConfig.txPin = jsonDoc["modbus"]["txpin"];
    } else {
        // EVSE-Wifi v2.0 uses hardwareserial on 22/21 (D1/D2)
        modbusConfig.enabled = true;
        modbusConfig.rxPin = 22;
        modbusConfig.txPin = 21;
    }
    sysLog.info(F("MODBUS loaded"), SysLog::CONFIG);

#endif

    // evseConfig
    evseConfig[0].mbid = 1;
#ifdef ESP32
    if (jsonDoc["evse"][0].containsKey("usemodbus"))
    {
        evseConfig[0].useModbus = jsonDoc["evse"][0]["usemodbus"];
        evseConfig[0].serialRxPin = jsonDoc["evse"][0]["serialrxpin"];
        evseConfig[0].serialTxPin = jsonDoc["evse"][0]["serialtxpin"];
    } else {
        // use defaults
        // v2.0 used modbus interface. Pins will be ignored here, if useModbus = true
        // otherwise a hardware serial with given pins will be used
        evseConfig[0].useModbus = true;
        evseConfig[0].serialRxPin = 22;   // not used when useModbus == true
        evseConfig[0].serialTxPin = 21;   // not used when useModbus == true
    }
#endif
    evseConfig[0].alwaysactive = jsonDoc["evse"][0]["alwaysactive"];

    evseConfig[0].resetcurrentaftercharge = jsonDoc["evse"][0]["resetcurrentaftercharge"];
    evseConfig[0].maxcurrent =  jsonDoc["evse"][0]["maxinstall"];
    evseConfig[0].avgconsumption = jsonDoc["evse"][0]["avgconsumption"];

    if (jsonDoc["evse"][0].containsKey("disableled")) {
        bool disableled = jsonDoc["evse"][0]["disableled"];
        sysLog.info(String("LED config: ") + disableled ? "disabled" : "enabled", SysLog::CONFIG);

        if (disableled == true) {
            evseConfig[0].ledconfig = 1;
        }
        else {
            evseConfig[0].ledconfig = 3;
        }
    }
    else{
        evseConfig[0].ledconfig = jsonDoc["evse"][0]["ledconfig"];
    }

    if (jsonDoc["evse"][0].containsKey("drotation")) {
        evseConfig[0].drotation = jsonDoc["evse"][0]["drotation"];
    }

    if (jsonDoc["evse"][0].containsKey("rsevalue")) {
        evseConfig[0].rseValue = jsonDoc["evse"][0]["rsevalue"];
    }
    else {
        evseConfig[0].rseValue = 100;
    }
    if (jsonDoc["evse"][0].containsKey("rseactive")) {
        evseConfig[0].rseActive = jsonDoc["evse"][0]["rseactive"];
    }
    else {
        evseConfig[0].rseActive = false;
    }
    if (jsonDoc["evse"][0].containsKey("remote")) {
        evseConfig[0].remote = jsonDoc["evse"][0]["remote"];
    }
    else {
        evseConfig[0].remote = false;
    }
    sysLog.info(F("EVSE loaded"), SysLog::CONFIG);
    sysLog.info(F("loadConfig.. Check!"), SysLog::CONFIG);
    configLoaded = true;

    return true;
}

bool ICACHE_FLASH_ATTR EvseWiFiConfig::loadConfiguration() {
    // meterConfig
    useMMeter = false;
    useSMeter = false;
    if (meterConfig[0].usemeter == true) {
        String type = meterConfig[0].metertype;
        sysLog.info("Use meter type: " + type, SysLog::CONFIG);
        if (type == "S0") {
            useSMeter = true;
        }
        else if (type == "SDM120") {
            mMeterTypeSDM120 = true;
            useMMeter = true;
        }
        else if (type == "SDM630") {
            mMeterTypeSDM630 = true;
            useMMeter = true;
        } else {
            sysLog.error("Unknown meter type: " + type, SysLog::CONFIG);
        }
    } else {
        sysLog.debug(F("no meter configured"), SysLog::CONFIG);
    } 

    return true;
}
bool ICACHE_FLASH_ATTR EvseWiFiConfig::printConfigFile() {
    //SPIFFS.begin();
    File configFile = SPIFFS.open(configFileName, "r");
    if (!configFile) {
        return false;
    }
    size_t size = configFile.size();
    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);
    DynamicJsonDocument jsonDoc(1800);
    DeserializationError error = deserializeJson(jsonDoc, buf.get());
    if (error) {
        return false;
    }
    String tmpJson;
    serializeJsonPretty(jsonDoc, tmpJson);
    sysLog.info(F("Config file:"), SysLog::CONFIG);
    sysLog.info(tmpJson, SysLog::CONFIG);
    //SPIFFS.end();
    return true;
}
bool ICACHE_FLASH_ATTR EvseWiFiConfig::printConfig() {

    // keep this as serial.print intentionally
    Serial.println("Printing Config... ---");
    Serial.println("// WiFi Config");
    Serial.println("bssid: " + String(getWifiBssid()));
    Serial.println("ssid: " + String(getWifiSsid()));
    Serial.println("wmode: " + String(getWifiWmode()));
    Serial.println("pswd: " + String(getWifiPass()));
    Serial.println("staticip: " + String(getWifiStaticIp()));
    Serial.println("ip: " + String(getWifiIp()));
    Serial.println("subnet: " + String(getWiFiSubnet()));
    Serial.println("gateway: " + String(getWiFiGateway()));
    Serial.println("dns: " + String(getWiFiDns()));
    Serial.println();
    Serial.println("// Meter Config");
    Serial.println("usemeter: " + String(getMeterActive(0)));
    Serial.println("metertype: " + String(getMeterType(0)));
    Serial.println("price: " + String(getMeterEnergyPrice(0)));
    Serial.println("intpin: " + String(getMeterPin(0)));
    Serial.println("kwhimp: " + String(getMeterImpKwh(0)));
    Serial.println("implen: " + String(getMeterImpLen(0)));
    Serial.println("meterphase: " + String(getMeterPhaseCount(0)));
    Serial.println("factor: " + String(getMeterFactor(0)));
    Serial.println();
    Serial.println("// RFID Config");
    Serial.println("userfid: " + String(getRfidActive()));
    Serial.println("sspin: " + String(getRfidPin()));
    Serial.println("rfidgain: " + String(getRfidGain()));
    Serial.println();
    Serial.println("// NTP Config");
    Serial.println("timezone: " + String(getNtpTimezone()));
    Serial.println("ntpip: " + String(getNtpIp()));
    Serial.println("dst: " + String(getNtpDst()));
    Serial.println();
    Serial.println("// Button Config");
    Serial.println("usebutton: " + String(getButtonActive(0)));
    Serial.println("buttonpin: " + String(getButtonPin(0)));
    Serial.println();
    Serial.println("// System Config");
    Serial.println("hostnm: " + String(getSystemHostname()));
    Serial.println("adminpwd: " + String(getSystemPass()));
    Serial.println("wsauth: " + String(getSystemWsauth()));
    Serial.println("debug: " + String(getSystemDebug()));
    Serial.println("maxinstall: " + String(getSystemMaxInstall()));
    Serial.println("evsecount: " + String(getSystemEvseCount()));
    Serial.println("logging: " + String(getSystemLogging()));
    Serial.println("api: " + String(getSystemApi()));
    Serial.println("configversion: " + String(getSystemConfigVersion()));
#ifdef ESP32
    Serial.println("// MODBUS Config");
    Serial.println("enabled: " + String(getModbusTxPin()));
    Serial.println("rxpin: " + String(getModbusRxPin()));
    Serial.println("txpin: " + String(getModbusRxPin()));
#endif
    Serial.println("// EVSE Config");
    Serial.println("mbid: " + String(getEvseMbid(0)));
#ifdef ESP32
    Serial.println("useModbus: " + String(getEvseUseModbus(0)));
    Serial.println("rxpin: " + String(getEvseSerialRxPin(0)));
    Serial.println("txpin: " + String(getEvseSerialTxPin(0)));
#endif
    Serial.println("alwaysactive: " + String(getEvseAlwaysActive(0)));
    Serial.println("remote: " + String(getEvseRemote(0)));
    Serial.println("ledconfig: " + String(getEvseLedConfig(0)));
    Serial.println("drotation: " + String(getEvseDisplayRotation(0)));
    Serial.println("resetcurrentaftercharge: " + String(getEvseResetCurrentAfterCharge(0)));
    Serial.println("evseinstall: " + String(getEvseMaxCurrent(0)));
    Serial.println("avgconsumption: " + String(getEvseAvgConsumption(0)));
    Serial.println("rseactive: " + String(getEvseRseActive(0)));
    Serial.println("rsevalue: " + String(getEvseRseValue(0)));
    Serial.println("--- End of Config...");
    return true;
}

bool ICACHE_FLASH_ATTR EvseWiFiConfig::renewConfigFile() {
    if (configLoaded) {
        if (getSystemConfigVersion() == ACTUAL_CONFIG_VERSION) {  //Config is up to date
            sysLog.info(F("Config file already up to date."), SysLog::CONFIG);
            return true;
        }
        else {  // Config must be updated
            sysLog.info(F("Config file has to be updated..."), SysLog::CONFIG);
            if (updateConfig(getConfigJson())) {
                return true;
            }
        }
    }
    return false;
}

String ICACHE_FLASH_ATTR EvseWiFiConfig::getConfigJson() {
    DynamicJsonDocument rootDoc(2000);
    rootDoc["configversion"] = ACTUAL_CONFIG_VERSION;
    #ifdef ESP8266
    rootDoc["hardwarerev"] = "ESP8266";
    #else
    rootDoc["hardwarerev"] = "ESP32";
    #endif

    JsonObject wifiItem = rootDoc.createNestedObject("wifi");
    wifiItem["bssid"] = this->getWifiBssid();
    wifiItem["ssid"] = this->getWifiSsid();
    wifiItem["wmode"] = this->getWifiWmode();
    wifiItem["pswd"] = this->getWifiPass();
    wifiItem["staticip"] = this->getWifiStaticIp();
    wifiItem["ip"] = this->getWifiIp();
    wifiItem["subnet"] = this->getWiFiSubnet();
    wifiItem["gateway"] = this->getWiFiGateway();
    wifiItem["dns"] = this->getWiFiDns();

    JsonArray meterArray = rootDoc.createNestedArray("meter");
    JsonObject meterObject_0 = meterArray.createNestedObject();
    meterObject_0["usemeter"] = this->getMeterActive(0);
    meterObject_0["metertype"] = this->getMeterType(0);
    meterObject_0["price"] = this->getMeterEnergyPrice(0);
    std::string sMeterType = this->getMeterType(0);
    if (sMeterType.substr(0,3) == "SDM") {
        meterObject_0["intpin"] = 0;
        meterObject_0["kwhimp"] = 0;
        meterObject_0["implen"] = 0;
    }
    else {
        //meterObject_0["intpin"] = this->getMeterPin(0);
        meterObject_0["kwhimp"] = this->getMeterImpKwh(0);
        meterObject_0["implen"] = this->getMeterImpLen(0);
    }
    meterObject_0["meterphase"] = this->getMeterPhaseCount(0);
    meterObject_0["factor"] = this->getMeterFactor(0);

    JsonObject rfidItem = rootDoc.createNestedObject("rfid");
    rfidItem["userfid"] = this->getRfidActive();
    rfidItem["sspin"] = this->getRfidPin();
    rfidItem["rfidgain"] = this->getRfidGain();

    JsonObject ntpItem = rootDoc.createNestedObject("ntp");
    ntpItem["timezone"] = this->getNtpTimezone();
    ntpItem["ntpip"] = this->getNtpIp();
    ntpItem["dst"] = this->getNtpDst();

    JsonArray buttonArray = rootDoc.createNestedArray("button");
    JsonObject buttonObject_0 = buttonArray.createNestedObject();
    buttonObject_0["usebutton"] = this->getButtonActive(0);
    buttonObject_0["buttonpin"] = this->getButtonPin(0);

    JsonObject systemItem = rootDoc.createNestedObject("system");
    systemItem["hostnm"] = this->getSystemHostname();
    systemItem["adminpwd"] = this->getSystemPass();
    systemItem["wsauth"] = this->getSystemWsauth();
    systemItem["debug"] = this->getSystemDebug();
    systemItem["maxinstall"] = this->getSystemMaxInstall();
    systemItem["evsecount"] = this->getSystemEvseCount();
    systemItem["logging"] = this->getSystemLogging();
    systemItem["api"] = this->getSystemApi();

#ifdef ESP32
    JsonObject modbusItem = rootDoc.createNestedObject("modbus");
    modbusItem["enabled"] = this->getModbusEnabled();
    modbusItem["rxpin"] = this->getModbusRxPin();
    modbusItem["txpin"] = this->getModbusTxPin();
#endif
    JsonArray evseArray = rootDoc.createNestedArray("evse");
    JsonObject evseObject_0 = evseArray.createNestedObject();
    evseObject_0["mbid"] = this->getEvseMbid(0);
#ifdef ESP32
    evseObject_0["usemodbus"] = this->getEvseUseModbus(0);
    evseObject_0["serialrxpin"] = this->getEvseSerialRxPin(0);
    evseObject_0["serialtxpin"] = this->getEvseSerialTxPin(0);
#endif
    evseObject_0["alwaysactive"] = this->getEvseAlwaysActive(0);
    evseObject_0["remote"] = this->getEvseRemote(0);
    evseObject_0["ledconfig"] = this->getEvseLedConfig(0);
    evseObject_0["drotation"] = this->getEvseDisplayRotation(0);
    evseObject_0["resetcurrentaftercharge"] = this->getEvseResetCurrentAfterCharge(0);
    evseObject_0["evseinstall"] = this->getEvseMaxCurrent(0);
    evseObject_0["avgconsumption"] = this->getEvseAvgConsumption(0);
    evseObject_0["rseactive"] = this->getEvseRseActive(0);
    evseObject_0["rsevalue"] = this->getEvseRseValue(0);

    String sReturn;
    serializeJsonPretty(rootDoc, sReturn);
    return sReturn;
}

bool ICACHE_FLASH_ATTR EvseWiFiConfig::checkUpdateConfig(String jsonString, bool (*setEVSERegister)(uint16_t reg, uint16_t val)) {
    DynamicJsonDocument jsonDoc(2000);
    DeserializationError error = deserializeJson(jsonDoc, jsonString);
    if (error) {
        sysLog.error("parsing config file failed: " + String(error.c_str()), SysLog::CONFIG);
        return false;
    }
    bool err = false;
    if (this->getEvseAlwaysActive(0) == true && jsonDoc["evse"][0]["alwaysactive"] == false) {  // switch from AA/Remote to Normal mode
        sysLog.debug(F("Switched from AA/Remote to normal mode -> going to set Register 2005..."), SysLog::CONFIG);
        for (int i = 0; i < 5; i++) {
            if (setEVSERegister(2005, 16448)){
                break;
            }
            err = true;
            delay(150);
        }
        if (err) {
            sysLog.error(F("Register 2005 could not be set (switch from AA/Remote to normal mode)"), SysLog::CONFIG);
            return false;
        } 
    }   
    else {
        sysLog.debug(F("No Register to Set (switch from AA/Remote to normal mode)"), SysLog::CONFIG);
    }
    return true;
}

bool ICACHE_FLASH_ATTR EvseWiFiConfig::updateConfig(String jsonString) {
    if(loadConfig(jsonString)) {
        if (saveConfigFile(jsonString)) {
            return true;
        }
    }
    return false;
}
bool ICACHE_FLASH_ATTR EvseWiFiConfig::saveConfigFile(String jsonString) {
    DynamicJsonDocument jsonDoc(1800);
    DeserializationError error = deserializeJson(jsonDoc, jsonString);
    if (error) return false;

    //SPIFFS.begin();
    File configFile = SPIFFS.open(configFileName, "w+");
    if (configFile) {
        if (jsonDoc.containsKey("command")) {
            jsonDoc.remove("command");
        }
        serializeJsonPretty(jsonDoc, configFile);
        configFile.close();

        //Check config file exists
        configFile = SPIFFS.open(configFileName, "r");
        if (configFile) {
            if (systemConfig.debug) {
                sysLog.debug(F("New config file created:"), SysLog::CONFIG);
                while (configFile.available()) {
                    Serial.write(configFile.read());
                }
                sysLog.debug(F("End of new config file"), SysLog::CONFIG);
            }
            else {
                sysLog.info(F("New config file created"), SysLog::CONFIG);
            }
            configFile.close();
            //SPIFFS.end();
            return true;
        }
    }
    //SPIFFS.end();
    return false;
}
uint8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getSystemConfigVersion() {
    if (systemConfig.configversion) return systemConfig.configversion;
    return 0;
}

// wifiConfig getter/setter
const char * ICACHE_FLASH_ATTR EvseWiFiConfig::getWifiBssid() {
    if (wifiConfig.bssid) return wifiConfig.bssid;
    return "";
}
const char * ICACHE_FLASH_ATTR EvseWiFiConfig::getWifiSsid() {
    if (wifiConfig.ssid) return wifiConfig.ssid;
    return "EVSE-WiFi";
}
bool ICACHE_FLASH_ATTR EvseWiFiConfig::getWifiWmode() {
    return wifiConfig.wmode;
}
const char * ICACHE_FLASH_ATTR EvseWiFiConfig::getWifiPass() {
    if (wifiConfig.pswd) return wifiConfig.pswd;
    return "";
}
bool ICACHE_FLASH_ATTR EvseWiFiConfig::getWifiStaticIp() {
    return wifiConfig.staticip;
}
const char * ICACHE_FLASH_ATTR EvseWiFiConfig::getWifiIp() {
    if (wifiConfig.ip) return wifiConfig.ip;
    return "";
}
const char * ICACHE_FLASH_ATTR EvseWiFiConfig::getWiFiSubnet() {
    if (wifiConfig.subnet) return wifiConfig.subnet;
    return "";
}
const char * ICACHE_FLASH_ATTR EvseWiFiConfig::getWiFiGateway() {
    if (wifiConfig.gateway) return wifiConfig.gateway;
    return "";
}
const char * ICACHE_FLASH_ATTR EvseWiFiConfig::getWiFiDns() {
    if (wifiConfig.dns) return wifiConfig.dns;
    return "";
}

// meterConfig getter/setter
bool ICACHE_FLASH_ATTR EvseWiFiConfig::getMeterActive(uint8_t meterId){
    return meterConfig[meterId].usemeter;
}
const char * ICACHE_FLASH_ATTR EvseWiFiConfig::getMeterType(uint8_t meterId){
    if (meterConfig[meterId].metertype) return meterConfig[meterId].metertype;
    return "";
}
float ICACHE_FLASH_ATTR EvseWiFiConfig::getMeterEnergyPrice(uint8_t meterId){
    if (meterConfig[meterId].price) return meterConfig[meterId].price;
    return 25;
}
uint8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getMeterPin(uint8_t meterId) {
    if (meterConfig[meterId].intpin) return meterConfig[meterId].intpin;
    #ifdef ESP8266
    return D3;
    #else
    return 17;
    #endif
}
uint16_t ICACHE_FLASH_ATTR EvseWiFiConfig::getMeterImpKwh(uint8_t meterId) {
    if (meterConfig[meterId].kwhimp) return meterConfig[meterId].kwhimp;
    return 1000;
}
uint16_t ICACHE_FLASH_ATTR EvseWiFiConfig::getMeterImpLen(uint8_t meterId) {
    if (meterConfig[meterId].implen) return (meterConfig[meterId].implen);
    return 30;
}
uint8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getMeterPhaseCount(uint8_t meterId) {
    if (meterConfig[meterId].meterphase) return meterConfig[meterId].meterphase;
    return 1;
}
uint8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getMeterFactor(uint8_t meterId) {
    if (meterConfig[meterId].factor) return meterConfig[meterId].factor;
    return 1;
}

// rfidConfig getter/setter
bool ICACHE_FLASH_ATTR EvseWiFiConfig::getRfidActive(){
    return rfidConfig.userfid;
}

uint8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getRfidUsePN532() {
    return false;
}

uint8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getRfidPin() {
    if (rfidConfig.sspin) return rfidConfig.sspin;
    #ifdef ESP8266
    return D8;
    #else
    return 5;
    #endif
}
int8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getRfidGain() {
    if (rfidConfig.rfidgain) return rfidConfig.rfidgain;
    return 112;
}

// ntpConfig getter/setter
int8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getNtpTimezone() {
    return ntpConfig.timezone;
}
const char * ICACHE_FLASH_ATTR EvseWiFiConfig::getNtpIp() {
    if (ntpConfig.ntpip) return ntpConfig.ntpip;
    return "pool.ntp.org";
}
bool ICACHE_FLASH_ATTR EvseWiFiConfig::getNtpDst() {
    return ntpConfig.dst;
}

// buttonConfig getter/setter
bool ICACHE_FLASH_ATTR EvseWiFiConfig::getButtonActive(uint8_t buttonId) {
    return buttonConfig[buttonId].usebutton;
}
uint8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getButtonPin(uint8_t buttonId) {
    if (buttonConfig[0].buttonpin) return buttonConfig[buttonId].buttonpin;
    #ifdef ESP8266
    return D4;
    #else
    return 16;
    #endif
}

// systemConfig getter/setter
const char * ICACHE_FLASH_ATTR EvseWiFiConfig::getSystemHostname() {
    if (systemConfig.hostnm) return systemConfig.hostnm;
    return "evse-wifi";
}
const char * ICACHE_FLASH_ATTR EvseWiFiConfig::getSystemPass() {
    if (systemConfig.adminpwd) return systemConfig.adminpwd;
    return "adminadmin";
}
bool ICACHE_FLASH_ATTR EvseWiFiConfig::getSystemWsauth() {
    return systemConfig.wsauth;
}
bool ICACHE_FLASH_ATTR EvseWiFiConfig::getSystemDebug() {
    return systemConfig.debug;
}
uint8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getSystemMaxInstall() {
    if (systemConfig.maxinstall) return systemConfig.maxinstall;
    return 10;
}
uint8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getSystemEvseCount() {
    return 1;
}
bool ICACHE_FLASH_ATTR EvseWiFiConfig::getSystemLogging() {
    return systemConfig.logging;
}
bool ICACHE_FLASH_ATTR EvseWiFiConfig::getSystemApi() {
    return systemConfig.api;
}

// evseConfig getter/setter
uint8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getEvseMbid(uint8_t evseId) {
    return evseConfig[evseId].mbid;
}
bool ICACHE_FLASH_ATTR EvseWiFiConfig::getEvseAlwaysActive(uint8_t evseId) {
    return evseConfig[evseId].alwaysactive;
}

#ifdef ESP32
bool ICACHE_FLASH_ATTR EvseWiFiConfig::getEvseUseModbus(uint8_t evseId) {
    return evseConfig[evseId].useModbus;
}
uint8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getEvseSerialRxPin(uint8_t evseId) {
    return evseConfig[evseId].serialRxPin;
}
uint8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getEvseSerialTxPin(uint8_t evseId) {
    return evseConfig[evseId].serialTxPin;
}
#endif

bool ICACHE_FLASH_ATTR EvseWiFiConfig::getEvseRemote(uint8_t evseId) {
    return evseConfig[evseId].remote;
}
uint8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getEvseLedConfig(uint8_t evseId) {
    return evseConfig[evseId].ledconfig;
}
uint8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getEvseDisplayRotation(uint8_t evseId) {
    return evseConfig[evseId].drotation;
}
uint8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getEvseLedPin(uint8_t evseId) {
    #ifdef ESP8266
    return D0;
    #else
    return 26;
    #endif
}
bool ICACHE_FLASH_ATTR EvseWiFiConfig::getEvseResetCurrentAfterCharge(uint8_t evseId) {
    return evseConfig[evseId].resetcurrentaftercharge;
}
uint8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getEvseMaxCurrent(uint8_t evseId) {
    return evseConfig[evseId].maxcurrent;
}
float ICACHE_FLASH_ATTR EvseWiFiConfig::getEvseAvgConsumption(uint8_t evseId) {
    if (evseConfig[evseId].avgconsumption) return evseConfig[0].avgconsumption;
    return 15.0;
}
uint8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getEvseCpIntPin(uint8_t evseId) {
    return 4;
}
bool ICACHE_FLASH_ATTR EvseWiFiConfig::getEvseRseActive(uint8_t evseId) {
    return evseConfig[evseId].rseActive;
}
uint8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getEvseRsePin(uint8_t evseId) {
    return 2;
}
uint8_t ICACHE_FLASH_ATTR EvseWiFiConfig::getEvseRseValue(uint8_t evseId) {
    return evseConfig[evseId].rseValue;
}