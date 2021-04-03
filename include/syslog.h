#pragma once
/**
 * logging system events to console
 */

#include <Arduino.h>

class SysLog {
public:
    // log levels to control verbosity
    enum Level_t {
        DEBUG = 0,
        INFO,
        WARNING,
        ERROR
    };

    // module enum to classify modules
    enum Module_t {
        SYSTEM = 0,
        MODBUS,
        EVSE,
        METER,
        WIFI,
        RFID,
        LOG,
        WEB,
        UPDT,
        CONFIG,
        AUTOCFG
    };

    SysLog(const SysLog::Level_t level = INFO);

    // dynamically change log level
    void setLogLevel(const Level_t level);

    // level specific log methods
    void debug(const String msg, const Module_t module = SysLog::SYSTEM);
    void info(const String msg, const Module_t module = SysLog::SYSTEM);
    void warning(const String msg, const Module_t module = SysLog::SYSTEM);
    void error(const String msg, const Module_t module = SysLog::SYSTEM);

private:
    Level_t logLevel;       // current log level
    // log method
    void log(String msg, const Module_t module, const Level_t level);
};

// singleton for logging
extern SysLog sysLog;