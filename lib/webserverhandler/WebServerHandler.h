/*
 * WebServerHandler.h
 *
 *  Created on: 11.03.2022
 *      Author: ToMe25
 */

#ifndef LIB_WEBSERVERHANDLER_H_
#define LIB_WEBSERVERHANDLER_H_

#include "GPIOHandler.h"
#include <ESPAsyncWebServer.h>

#define HTML_BINARY "_binary_lib_webserverhandler_html_"

extern const char INDEX_HTML[] asm(HTML_BINARY "index_html_start");
extern const char INDEX_STATE_HTML[] asm(HTML_BINARY "index_state_html_start");
extern const char SETTINGS_HTML[] asm(HTML_BINARY "settings_html_start");
extern const char SETTINGS_PIN_HTML[] asm(HTML_BINARY "settings_pin_html_start");
extern const char DELETE_HTML[] asm(HTML_BINARY "delete_html_start");
extern const char NOT_FOUND_HTML[] asm(HTML_BINARY "not_found_html_start");
extern const char MAIN_CSS[] asm(HTML_BINARY "main_css_start");
extern const char INDEX_JS[] asm(HTML_BINARY "index_js_start");
extern const char SETTINGS_JS[] asm(HTML_BINARY "settings_js_start");

class WebServerHandler {
public:
	/**
	 * The default constructor for creating a new WebServerHandler.
	 *
	 * @param port	The port on which the web server should listed.
	 */
	WebServerHandler(const uint16_t port = 80, GPIOHandler &gpio = gpio_handler);
	virtual ~WebServerHandler();

	/**
	 * Initializes the web server.
	 * Adds uri handlers and similar.
	 * Also adds a mDNS txt for the web server.
	 *
	 * Same as begin.
	 */
	void setup();

	/**
	 * Initializes the web server.
	 * Adds uri handlers and similar.
	 * Also adds a mDNS txt for the web server.
	 *
	 * Same as setup.
	 */
	void begin();

	/**
	 * Stops the web server and removes the mDNS txt.
	 */
	void end();

	/**
	 * Sets the GPIOHandler to use.
	 * Both to read the pin states from, and write pin changes to.
	 *
	 * @param gpio	The new GPIOHandler to use.
	 */
	void setGPIOHandler(GPIOHandler &gpio);

	/**
	 * Gets the currently used GPIOHandler.
	 *
	 * @return	The current GPIOHandler.
	 */
	GPIOHandler& getGPIOHandler() const;
private:
	/**
	 * The port on which this web server listens to http requests.
	 */
	const uint16_t _port;

	/**
	 * The underlying web server handling the web requests.
	 */
	AsyncWebServer server;

	/**
	 * The GPIOHandler from which to get the pins to display, and on which to register/update pins.
	 * Cannot be NULL unless something goes wrong.
	 */
	GPIOHandler *gpio;

	/**
	 * The method responding to http requests for the prometheus metrics endpoint.
	 *
	 * @param request	The request to handle.
	 */
	void getMetrics(AsyncWebServerRequest *request) const;

	/**
	 * The method handling requests to pages that don't exist.
	 *
	 * @param request	The request to handle.
	 */
	void onNotFound(AsyncWebServerRequest *request) const;

	/**
	 * The method generating final the index.html page and sending it to the client.
	 *
	 * @param request	The http request to handle.
	 */
	void getIndex(AsyncWebServerRequest *request) const;

	/**
	 * The method generating the final settings.html page and sending it to the client.
	 * As well as handling changes requested by the client.
	 *
	 * @param request	The web request to handle.
	 */
	void handleSettings(AsyncWebServerRequest *request) const;

	/**
	 * The method responding to http requests for the confim pin deletion page.
	 *
	 * @param request	The web request to handle.
	 */
	void postDelete(AsyncWebServerRequest *request) const;

	/**
	 * The method for handling get requests for the pins.json file.
	 *
	 * @param request	The request to handle.
	 */
	void getPinsJson(AsyncWebServerRequest *request) const;
};

#endif /* LIB_WEBSERVERHANDLER_H_ */
