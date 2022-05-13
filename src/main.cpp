/*
 * main.cpp
 *
 *  Created on: 05.03.2022
 *      Author: ToMe25
 */

#include "main.h"
#include "GPIOHandler.h"
#include "StorageHandler.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

void setup() {
	start = millis();
	Serial.begin(115200);

	WiFi.disconnect(true);

	WiFi.onEvent(onWiFiStart, SYSTEM_EVENT_STA_START);
	WiFi.onEvent(onWiFiConnected, SYSTEM_EVENT_STA_CONNECTED);
	if (!STATIC_IP_ENABLED) {
		WiFi.onEvent(onWiFiGotIp, SYSTEM_EVENT_STA_GOT_IP);
	}

	WiFi.mode(WIFI_STA);

	WiFi.setAutoReconnect(true);
	WiFi.setSleep(false);

	if (STATIC_IP_ENABLED) {
		WiFi.config(STATIC_IP, GATEWAY_IP, NETMASK);
	}

	WiFi.begin(WIFI_SSID, WIFI_PASS);

	gpio_handler.setDebounceTimeout(PIN_DEBOUNCE_TIMEOUT);

	bool spiffs = SPIFFS.begin(true);
	if (!spiffs) {
		Serial.println("Warning: Initializing SPIFFS failed, persistent storage will not be available!");
	}

	if (spiffs) {
		storage_handler.loadGPIOHandler(gpio_handler);
	} else {
		storage_handler.setPinStoragePath(NULL);
	}

	setupOTA();
	server.setup();

	bool wifiReady = ready != 0;
	ready = millis();

	if (wifiReady) {
		Serial.print("ESP ready after ");
		Serial.print(ready - start);
		Serial.println("ms.");
	}
}

void loop() {
	delay(20);

	ArduinoOTA.handle();

	uint64_t now = millis();
	if (now - last_pin_check > PIN_CHECK_INTERVAL) {
		last_pin_check = now;
		gpio_handler.checkPins();
		gpio_handler.writeToStorageHandler();
	}
}

void setupOTA() {
	ArduinoOTA.setHostname(HOSTNAME);
	ArduinoOTA.setPassword(OTA_PASS);

	ArduinoOTA.onStart([]() {
		Serial.println("Start updating sketch.");
		gpio_handler.disableInterrupts();
	});

	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	});

	ArduinoOTA.onEnd([]() {
		Serial.println("\nUpdate Done.");
	});

	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("OTA Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) {
			Serial.println("Auth Failed.");
		} else if (error == OTA_BEGIN_ERROR) {
			Serial.println("Begin Failed.");
		} else if (error == OTA_CONNECT_ERROR) {
			Serial.println("Connect Failed.");
		} else if (error == OTA_RECEIVE_ERROR) {
			Serial.println("Receive Failed.");
		} else if (error == OTA_END_ERROR) {
			Serial.println("End Failed.");
		}
	});

	ArduinoOTA.begin();
}

void onWiFiStart(WiFiEvent_t event, WiFiEventInfo_t eventInfo) {
	WiFi.setHostname(HOSTNAME);
}

void onWiFiConnected(WiFiEvent_t event, WiFiEventInfo_t eventInfo) {
	WiFi.enableIpV6();
	if (STATIC_IP_ENABLED) {
		Serial.print("IP Address: ");
		Serial.println(localhost = STATIC_IP);

		bool setupReady = ready != 0;

		ready = millis();

		if (setupReady) {
			Serial.print("ESP ready after ");
		} else {
			Serial.print("WiFi ready after ");
		}
		Serial.print(ready - start);
		Serial.println("ms.");
	}
}

void onWiFiGotIp(WiFiEvent_t event, WiFiEventInfo_t eventInfo) {
	localhost = (IPAddress) eventInfo.got_ip.ip_info.ip.addr;
	Serial.print("IP Address: ");
	Serial.println(localhost);

	bool setupReady = ready != 0;

	ready = millis();

	if (setupReady) {
		Serial.print("ESP ready after ");
	} else {
		Serial.print("WiFi ready after ");
	}
	Serial.print(ready - start);
	Serial.println("ms.");
}
