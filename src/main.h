/*
 * main.h
 *
 *  Created on: 05.03.2022
 *      Author: ToMe25
 */

#ifndef SRC_MAIN_H_
#define SRC_MAIN_H_

#include "config.h"
#include "WebServerHandler.h"

extern const char WIFI_SSID[] asm("_binary_wifissid_txt_start");
extern const char WIFI_PASS[] asm("_binary_wifipass_txt_start");
extern const char OTA_PASS[] asm("_binary_otapass_txt_start");

/**
 * The method that gets called once on startup and initializes everything.
 */
void setup();

/**
 * The main loop that gets called repeatedly until the ESP stops.
 */
void loop();

/**
 * The method that initializes everything related to ArduinoOTA.
 */
void setupOTA();

/**
 * A callback handling everything to be done when the WiFi station is starting.
 *
 * @param event		The id of the WiFi event to handle.
 * @param eventInfo	Additional context for the event to handle.
 */
void onWiFiStart(WiFiEvent_t event, WiFiEventInfo_t eventInfo);

/**
 * A callback method called when the ESP connects to a WiFi access point.
 *
 * @param event		The id of the WiFi event to handle.
 * @param eventInfo	Additional context for the event to handle.
 */
void onWiFiConnected(WiFiEvent_t event, WiFiEventInfo_t eventInfo);

/**
 * A callback for when the ESP gets an IPv4 address.
 * Only called if this ESP doesn't have a static IP address.
 *
 * @param event		The id of the WiFi event to handle.
 * @param eventInfo	Additional context for the event to handle.
 */
void onWiFiGotIp(WiFiEvent_t event, WiFiEventInfo_t eventInfo);

/**
 * Whether this ESP uses a static IP address or a dhcp one.
 */
const bool STATIC_IP_ENABLED = STATIC_IP != IPADDR_ANY || GATEWAY_IP != IPADDR_ANY || NETMASK != IPADDR_ANY;

/**
 * The system time in milliseconds at which the setup function was first called.
 */
uint64_t start;

/**
 * This system time in milliseconds at which the ESP was first fully ready.
 */
uint64_t ready = 0;

/**
 * The time of the last manual pin check.
 */
uint64_t last_pin_check = 0;

/**
 * The local IPv4 address of this ESP.
 */
IPAddress localhost;

/**
 * The web server used to view the current pin states and configure pins to watch.
 */
WebServerHandler server(WEBSERVER_PORT);

#endif /* SRC_MAIN_H_ */
