/*
 * ntp.h
 *
 *  Created on: 8 Mar 2016
 *      Author: joe
 */

#pragma once

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESPAsyncUDP.h>
#else
#include <WiFi.h>
#include <AsyncUDP.h>
#endif

#include <TimeLib.h>

#define NTP_PACKET_SIZE 48

struct deviceUptime {
	uint32_t days;
	uint32_t hours;
	uint32_t mins;
	uint32_t secs;
};

class NtpClient {
public:

	void ICACHE_FLASH_ATTR Ntp(const char * server, int8_t tz, time_t syncSecs);
	ICACHE_FLASH_ATTR virtual ~NtpClient();

	static char * TimeServerName;
	static IPAddress timeServer;
	static int8_t timezone;
	static time_t syncInterval;

	static AsyncUDP udpListener;

	static byte NTPpacket[NTP_PACKET_SIZE];

	static ICACHE_FLASH_ATTR String iso8601DateTime();
	static ICACHE_FLASH_ATTR deviceUptime getDeviceUptime();
	static ICACHE_FLASH_ATTR String getDeviceUptimeString();
	static ICACHE_FLASH_ATTR time_t getUtcTimeNow();
	bool ICACHE_FLASH_ATTR processTime();
	time_t getUptimeSec();

private:
	static ICACHE_FLASH_ATTR String zeroPaddedIntVal(int val);
	static ICACHE_FLASH_ATTR time_t getNtpTime();

protected:
	time_t _uptimesec = 0;
};

// singleton
extern NtpClient ntp;
