/*
 * web_server_test.h
 *
 *  Created on: 11.03.2022
 *      Author: ToMe25
 */

#ifndef TEST_WEB_SERVER_TEST_H_
#define TEST_WEB_SERVER_TEST_H_

#include "WebServerHandler.h"
#include <HTTPClient.h>

/**
 * The webserver handler instance to test.
 */
extern WebServerHandler web_server;

/**
 * The http client used to request pages from the web server.
 */
extern HTTPClient client;

/**
 * Splits the given page into individual strings, one for each line.
 * Also removes leading and trailing whitespaces from the lines.
 *
 * @param page	The page to split into its lines.
 * @return	A vector containing one string per line of input.
 */
std::unique_ptr<std::vector<std::string>> split_lines(const char *page);

/**
 * Tests whether the give metrics are generated correctly.
 * Checks whether there is a help text, a correct type line, and a correct metric line.
 *
 * @param metrics_page	The content returned by the metrics endpoint.
 * @param metric_name	The name of the metric to test.
 * @param pin_nr		The pin for which to check the metric.
 * @param pin_name		The name of the pin for which to check the generated metric.
 * @param metric_type	The type of the metric to check.
 * @param value			The expected value of the metric for the given pin to check.
 */
void check_pin_metric(const char *metrics_page, const char *metric_name, const uint8_t pin_nr,
		const char *pin_name, const char *metric_type, const uint64_t value);

/**
 * Checks whether the given pins json contains a correct line for the given pin.
 *
 * @param pins_json	The json page to check.
 * @param pin_nr	The pin to check for.
 * @param pin_name	The name of the pin to look for.
 * @param pull_up	Whether the pin to look for uses an internal pull up resistor instead of a pull down one.
 * @param state		The current state of the pin to look for.
 * @param changes	The number of state changes of the pin to check for.
 */
void check_json_pin(const char *pins_json, const uint8_t pin_nr,
		const char *pin_name, const bool pull_up, const bool state,
		const uint64_t changes);

/**
 * Checks whether the given line is a valid index pin line for the given value type.
 *
 * @param line		The line to check.
 * @param line_type	The value type to check for (valid values: Name, Pin, Resistor, State, and Changes)
 * @param value		The expected value for the given line.
 */
void check_index_pin_line(const std::string *line, const char *line_type, const uint64_t value);

/**
 * Checks whether the given line is a valid index pin line for the given value type.
 *
 * @param line		The line to check.
 * @param line_type	The value type to check for (valid values: Name, Pin, Resistor, State, and Changes)
 * @param value		The expected value for the given line.
 */
void check_index_pin_line(const std::string *line, const char *line_type, const char *value);

/**
 * Tests whether the pin section in the index.html page for the given pin was generated correctly.
 *
 * @param index_page	The page returned by a get request to index.html.
 * @param pin_nr		The pin for which to check the pin section.
 * @param pin_name		The name of the pin to check.
 * @param pull_up		Whether the pin has a pull up resistor or a pull down resistor.
 * @param state			Whether the pin is currently high.
 * @param changes		The number of state changes for the pin to check.
 */
void check_index_pin(const char *index_page, const uint8_t pin_nr,
		const char *pin_name, const bool pull_up, const bool state,
		const uint64_t changes);

/**
 * Tests whether the generated pin section in the settings.html page for the given pin is correct.
 *
 * @param settings_page	The page returned by a request to /settings.html.
 * @param pin_nr		The pin for which to check the pin section.
 * @param name			The name of the pin for which to check the pin section.
 * @param pull_up		Whether the pin has a pull up resistor or a pull down resistor.
 * @param state			Whether the pin is currently high.
 * @param changes		The number of state changes for the pin to check.
 */
void check_settings_pin(const char *settings_page, const uint8_t pin_nr,
		const char *name, const bool pull_up, const bool state,
		const uint64_t changes);

/**
 * Checks whether a page contains the correct message on its top.
 *
 * @param page			The html page to check.
 * @param message_type	The html class of the message tag.
 * @param message		The message that should be displayed on the page.
 * @param hidden		Whether the message is invisible using the html hidden property.
 */
void check_message(const char *page, const char *message_type,
		const char *message, const bool hidden);

/**
 * Tests whether the delete.html page is generated correctly.
 *
 * @param delete_page	The page to check for the pin values.
 * @param pin_nr		The pin that should be found on the page.
 * @param name			The name of the pin to check for.
 * @param pull_up		Whether the pin should have a pull up resistor.
 * @param state			The current expected state of the pin.
 * @param changes		How many times the pin state should have changed already.
 */
void check_delete_pin(const char *delete_page, const uint8_t pin_nr,
		const char *name, const bool pull_up, const bool state,
		const uint64_t changes);

/**
 * Initializes the WiFi interface in AP mode to test web server functionality, as well as the web server handler.
 */
void init_web_server();

/**
 * The method running all the WebServerHandler tests.
 */
void run_webserver_tests();

/**
 * Test whether all the static pages(css and javascript) return the correct status code and content type.
 */
void test_static_pages();

/**
 * Tests the functionality of the prometheus metrics endpoint.
 */
void test_metrics_endpoint();

/**
 * Tests whether pins.json is generated correctly.
 */
void test_pins_json();

/**
 * Tests whether the index.html page contains the correct pin info.
 */
void test_index_html();

/**
 * Tests the settings.html page.
 * Makes sure it contains the correct pin info, reacts correctly to posts, and contains the correct message.
 */
void test_settings_html();

/**
 * Tests whether the delete page works correctly.
 */
void test_delete_html();

#endif /* TEST_WEB_SERVER_TEST_H_ */
