/*
 * config.h
 *
 *  Created on: 05.03.2022
 *      Author: ToMe25
 */

#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_

#include <WiFi.h>

/**
 * The hostname for this device.
 * Used both as the hostname and as the mDNS name.
 */
static constexpr char HOSTNAME[] = "esp-wifi-gpio-monitor";

/**
 * The local ip address for this ESP.
 * Set to IPADDR_ANY to use a dhcp provided ip.
 */
static const IPAddress STATIC_IP = IPADDR_ANY;

/**
 * The ip address of the network gateway(usually your router).
 * Set to IPADDR_ANY to use the one suggested by dhcp.
 * Might be required if a static ip is used.
 */
static const IPAddress GATEWAY_IP = IPADDR_ANY;

/**
 * The netmask to of the local network to connect to.
 * Set to IPADDR_ANY to use the default.
 */
static const IPAddress NETMASK = IPADDR_ANY;

/**
 * The port for the web server to listen on.
 */
static const uint16_t WEBSERVER_PORT = 80;

/**
 * The interval for manual pin state checks.
 * This program checks all pins once at startup, and uses interrupts to immediately notice pin changes.
 * However as a fallback it also checks the pin state for all watched pins every so often.
 * This is the time for this check.
 * In milliseconds.
 * Default is 5 minutes.(5 * 60 * 1000 = 300000)
 */
static const uint64_t PIN_CHECK_INTERVAL = 300000;

/**
 * The time a pin has to be in the same state for it to be considered changed.
 * Used to prevent pin change counts to increase a lot for each button press.
 * In milliseconds.
 */
static const uint16_t PIN_DEBOUNCE_TIMEOUT = 10;

#endif /* SRC_CONFIG_H_ */
