#include <syslog.h>
#include <ntp.h>

// singleton for logging
SysLog sysLog(SysLog::INFO);

SysLog::SysLog(const Level_t level) : logLevel(level){};

void SysLog::setLogLevel(const Level_t level) {
    logLevel = level;
};

void SysLog::debug(const String msg, const Module_t module) {
    log(msg, module, DEBUG);
};

void SysLog::info(const String msg, const Module_t module) {
    log(msg, module, INFO);
};

void SysLog::warning(const String msg, const Module_t module) {
    log(msg, module, WARNING);
};

void SysLog::error(const String msg, const Module_t module) {
    log(msg, module, ERROR);
};

void SysLog::log(const String msg, const Module_t module, const Level_t level) {
    // dont log if too low level
    if (level <  logLevel ) return;
    // log time
    time_t tm = ntp.getUtcTimeNow();
    // if synchronized, use timestamp, otherwise uptime
    char buff[20];
    if( tm > 10*12*30*24*3600 ) { // assume any date > 10yrs after 1970 means ntp synchronized
        strftime(buff, 20, "(%Y-%m-%d %H:%M:%S)", localtime(&tm));
    } else {
        deviceUptime uptime = ntp.getDeviceUptime();
        sprintf(buff, "(%ud %02d:%02d:%02d)", uptime.days, uptime.hours, uptime.mins, uptime.secs);
    }
    String prefix(buff);
    switch( level ) {
        case DEBUG: prefix += " debug"; break;
        case INFO: prefix += " info"; break;
        case WARNING: prefix += " warning"; break;
        case ERROR: prefix += " error"; break;
        default: prefix += "";
    }
    switch( module ) {
        case SYSTEM: prefix += " [SYSTEM]: "; break;
        case MODBUS: prefix += " [MODBUS]: "; break;
        case EVSE: prefix += " [EVSE]: "; break;
        case METER: prefix += " [METER]: "; break;
        case RFID: prefix += " [RFID]: "; break;
        case WIFI: prefix += " [WIFI]: "; break;
        case LOG: prefix += " [LOG]: "; break;
        case WEB: prefix += " [WEB]: "; break;
        case UPDT: prefix += " [UPDT]: "; break;
        case CONFIG: prefix += " [CONFIG]: "; break;
        case AUTOCFG: prefix += " [Auto-Config]:"; break;
        default: prefix +=  ": ";
    }

    Serial.println(prefix + msg);
}