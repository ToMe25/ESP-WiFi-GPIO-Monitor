/*
 * web_server_test.cpp
 *
 *  Created on: 11.03.2022
 *      Author: ToMe25
 */

#include "web_server_test.h"
#include "test_main.h"
#include "GPIOHandler.h"
#include <unity.h>
#include <ESPmDNS.h>
#include <HTTPClient.h> // Somehow this include is required for the one in the header to work

WebServerHandler web_server;
HTTPClient client;

std::unique_ptr<std::vector<std::string>> split_lines(const char *page) {
	size_t lines_count = 0;
	for (size_t i = 0; i < strlen(page); i++) {
		if (page[i] == '\n') {
			lines_count++;
		}
	}
	std::unique_ptr<std::vector<std::string>> lines(
			new std::vector<std::string>());
	lines->reserve(lines_count + 1);

	std::unique_ptr<std::istringstream> stream(new std::istringstream(page));
	for (std::string line; std::getline(*stream, line);) {
		lines->push_back(line);
	}

	for (size_t i = 0; i < lines->size(); i++) {
		(*lines)[i] = std::regex_replace((*lines)[i], std::regex("^\\s+"), "");
		(*lines)[i] = std::regex_replace((*lines)[i], std::regex("\\s+$"), "");
	}

	return lines;
}

void check_pin_metric(const char *metrics_page, const char *metric_name,
		const uint8_t pin_nr, const char *pin_name, const char *metric_type,
		const uint64_t value) {
	// Separate metrics page into separate lines.
	const std::unique_ptr<std::vector<std::string>> lines = split_lines(
			metrics_page);

	// Test whether there are at least 3 lines.
	TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(3, lines->size(),
			"The metrics endpoint did not return 3 or more lines.");

	// Make sure there is a help line followed by a correct type line.
	std::string help_start("# HELP ");
	help_start += metric_name;
	size_t i = 0;
	while (i < lines->size()) {
		if ((*lines)[i].substr(0, help_start.length()) == help_start) {
			break;
		}
		i++;
	}
	TEST_ASSERT_LESS_THAN_MESSAGE(lines->size() - 2, i,
			std::string("Could not find help line for metric ").append(
					metric_name).append(".").c_str());
	TEST_ASSERT_EQUAL_STRING_MESSAGE(
			std::string("# TYPE ").append(metric_name).append(" ").append(
					metric_type).c_str(), (*lines)[i + 1].c_str(),
			std::string(
					"The line after the help line was not the type string for ").append(
					metric_name).append(".").c_str());

	// Test the metric line for the metric to check.
	std::unique_ptr<std::ostringstream> metric_ln(new std::ostringstream());
	*metric_ln << metric_name;
	*metric_ln << "{pin=\"" << (uint16_t) pin_nr;
	*metric_ln << "\",name=\"" << pin_name;
	*metric_ln << "\"}";
	std::string metric_line = (*metric_ln).str();
	for (i += 2; i < lines->size(); i++) {
		if ((*lines)[i].substr(0, metric_line.length()) == metric_line) {
			break;
		} else if ((*lines)[i][0] == '#') {
			std::ostringstream error;
			error << "Couldn't find a valid metric line for metric ";
			error << metric_name;
			error << " and pin " << (uint16_t) pin_nr;
			error << '(' << pin_name << ')';
			TEST_FAIL_MESSAGE(error.str().c_str());
		}
	}

	// Make sure the value of the metric is correct.
	TEST_ASSERT_EQUAL_MESSAGE(value,
			atoi(
					(*lines)[i].substr((*lines)[i].find_last_of(' '),
							(*lines)[i].length() - 1).c_str()),
			std::string("The value of ").append(metric_line).append(
					" isn't correct.").c_str());
}

void check_json_pin(const char *pins_json, const uint8_t pin_nr,
		const char *pin_name, const bool pull_up, const bool state,
		const uint64_t changes) {
	// Make sure the returned json object starts and ends with curly brackets.
	const std::unique_ptr<std::vector<std::string>> lines = split_lines(
			pins_json);
	TEST_ASSERT_EQUAL_MESSAGE('{', (*lines)[0][0],
			"The pins json did not start with a curly bracket.");
	TEST_ASSERT_EQUAL_MESSAGE('}',
			(*lines)[lines->size() - 1][(*lines)[lines->size() - 1].length() - 1],
			"The pins json did not end with a curly bracket.");

	// Construct the key value pair to check against.
	std::unique_ptr<std::ostringstream> stream(new std::ostringstream);
	*stream << '"' << (uint16_t) pin_nr;
	*stream << "\": {\"pin\": " << (uint16_t) pin_nr;
	*stream << ", \"name\": \"" << pin_name;
	*stream << "\", \"pull_up\": " << (pull_up ? "true" : "false");
	*stream << ", \"state\": \"" << (state ? "High" : "Low");
	*stream << "\", \"changes\": " << changes;
	*stream << '}';

	// Test whether any line matches the given pin.
	for (size_t i = 0; i < lines->size(); i++) {
		if (stream->str()
				== (*lines)[i].substr(i == 0,
						(*lines)[i].length() - 1 - (i == 0))) {
			return;
		}
	}

	TEST_FAIL_MESSAGE(
			"Could not find a line in the json that matches the given parameters.");
}

void check_index_pin_line(const std::string *line, const char *line_type,
		const uint64_t value) {
	std::unique_ptr<std::ostringstream> str(new std::ostringstream());
	*str << value;
	check_index_pin_line(line, line_type, str->str().c_str());
}

void check_index_pin_line(const std::string *line, const char *line_type,
		const char *value) {
	std::unique_ptr<std::ostringstream> str(new std::ostringstream());
	*str << "<span class=\"value\">" << line_type;
	*str << ": <output name=\"" << (char) std::tolower(line_type[0])
			<< (line_type + 1);
	if (strcmp(line_type, "State") == 0) {
		*str << "\" class=\"" << value << "color";
	}
	*str << "\">" << value << "</output></span>";

	TEST_ASSERT_EQUAL_STRING_MESSAGE(str->str().c_str(),
			line->substr(0, str->str().length()).c_str(),
			std::string("The ").append(line_type).append(
					" line for the given pin was not correct.").c_str());
}

void check_index_pin(const char *index_page, const uint8_t pin_nr,
		const char *pin_name, const bool pull_up, const bool state,
		const uint64_t changes) {
	// Split page into its lines.
	const std::unique_ptr<std::vector<std::string>> lines = split_lines(
			index_page);

	size_t i = 0;
	for (; i < lines->size(); i++) {
		if ((*lines)[i] == "<div class=\"state\">") {
			// Confirm there being enough lines left for a valid pin section.
			TEST_ASSERT_LESS_THAN_MESSAGE(lines->size(), i + 6,
					"There are not enough lines left in the index.html page after the start of a pin section.");

			// Test that the second line is a pin number line.
			std::unique_ptr<std::regex> pin_nr_line(
					new std::regex(
							"<span class=\"value\">Pin: <output name=\"pin\">\\d{1,2}</output></span>"));

			TEST_ASSERT_MESSAGE(std::regex_match((*lines)[i + 2], *pin_nr_line),
					"The second line after the start of the pin section was not a pin number line.");
			pin_nr_line.reset();

			// Make sure this pin section is for the correct pin.
			std::unique_ptr<std::ostringstream> str(new std::ostringstream());
			*str << "<span class=\"value\">Pin: <output name=\"pin\">";
			*str << (uint16_t) pin_nr << "</output></span>";
			if (str->str() == (*lines)[i + 2]) {
				// Make sure all lines are generated correctly.
				check_index_pin_line(&(*lines)[++i], "Name", pin_name);
				check_index_pin_line(&(*lines)[++i], "Pin", (uint64_t) pin_nr);
				check_index_pin_line(&(*lines)[++i], "Resistor",
						pull_up ? "Pull Up" : "Pull Down");
				check_index_pin_line(&(*lines)[++i], "State",
						state ? "High" : "Low");
				check_index_pin_line(&(*lines)[++i], "Changes", changes);

				return;
			} else {
				continue;
			}
		}
	}

	std::unique_ptr<std::ostringstream> str(new std::ostringstream());
	*str << "The pin section for pin ";
	*str << (uint16_t) pin_nr;
	*str << " could not be found in the given index.html page.";
	TEST_FAIL_MESSAGE(str->str().c_str());
}

void check_settings_pin(const char *settings_page, const uint8_t pin_nr,
		const char *name, const bool pull_up, const bool state,
		const uint64_t changes) {
	// Split page into its lines.
	const std::unique_ptr<std::vector<std::string>> lines = split_lines(
			settings_page);

	// The correct lines for all those lines that do not contain dynamic info.
	std::unique_ptr<std::vector<std::string>> correct_lines = split_lines(
			SETTINGS_PIN_HTML);

	// Check for the pin block for the given pin.
	size_t i = 0;
	for (; i < lines->size(); i++) {
		if ((*lines)[i]
				== "<form class=\"state\" method=\"post\" action=\"/settings.html\">") {
			// Confirm there being enough lines left for a valid pin section.
			TEST_ASSERT_LESS_THAN_MESSAGE(lines->size(), i + 21,
					"There are not enough lines left in the settings.html page after the start of a pin section.");

			// Make sure this pin section is for the correct pin.
			std::unique_ptr<std::ostringstream> str(new std::ostringstream());
			*str << "name=\"pin\" value=\"";
			*str << (uint16_t) pin_nr;
			*str
					<< "\" max=\"39\" min=\"0\" title=\"The hardware pin to watch.\"";
			if (str->str() == (*lines)[i + 7]) {
				for (uint8_t j = 1; j < 22; j++) {
					std::string line;
					switch (j) {
					case 2:
						*str = std::ostringstream();
						*str << "name=\"name\" value=\"";
						*str << name << '"';
						line = str->str();
						break;
					case 7:
						*str = std::ostringstream();
						*str << "name=\"pin\" value=\"";
						*str << (uint16_t) pin_nr;
						*str
								<< "\" max=\"39\" min=\"0\" title=\"The hardware pin to watch.\"";
						line = str->str();
						break;
					case 12:
						*str = std::ostringstream();
						*str
								<< "alt=\"The pin will use an internal pull up resistor.\" ";
						if (pull_up) {
							*str << "checked";
						}
						*str << "><label>Pull";
						line = str->str();
						break;
					case 15:
						*str = std::ostringstream();
						*str
								<< "alt=\"The pin will use an internal pull down resistor.\" ";
						if (!pull_up) {
							*str << "checked";
						}
						*str << "><label>Pull";
						line = str->str();
						break;
					case 17:
						*str = std::ostringstream();
						*str
								<< "<span class=\"value\">State: <output name=\"state\" class=\"";
						*str << (state ? "High" : "Low");
						*str << "color\">";
						*str << (state ? "High" : "Low");
						*str << "</output></span>";
						line = str->str();
						break;
					case 18:
						*str = std::ostringstream();
						*str
								<< "<span class=\"value\">Changes: <output name=\"changes\">";
						*str << changes;
						*str << "</output></span><br />";
						line = str->str();
						break;
					default:
						line = (*correct_lines)[j];
					}

					TEST_ASSERT_EQUAL_STRING_MESSAGE(line.c_str(),
							(*lines)[i + j].c_str(),
							"The current line in the pin section didn't match what it should have been.");
				}

				return;
			} else {
				continue;
			}
		}
	}

	std::unique_ptr<std::ostringstream> str(new std::ostringstream());
	*str << "The pin section for pin ";
	*str << (uint16_t) pin_nr;
	*str << " could not be found in the given settings.html page.";
	TEST_FAIL_MESSAGE(str->str().c_str());
}

void check_message(const char *page, const char *message_type,
		const char *message, const bool hidden) {
	// Split page into its lines.
	const std::unique_ptr<std::vector<std::string>> lines = split_lines(page);

	// Find the message line in the settings page.
	for (size_t i = 0; i < lines->size(); i++) {
		if ((*lines)[i] == "<div class=\"main\">") {
			TEST_ASSERT_LESS_THAN_MESSAGE(lines->size(), i + 1,
					"There doesn't seem to be a message line.");
			std::unique_ptr<std::ostringstream> str(new std::ostringstream);
			*str << "<div class=\"message ";
			*str << message_type;
			*str << '"';
			if (hidden) {
				*str << " hidden";
			}
			*str << '>';
			*str << message;
			*str << "</div>";
			TEST_ASSERT_EQUAL_STRING_MESSAGE(str->str().c_str(),
					(*lines)[i + 1].c_str(),
					"The message line did not match what it should have been.");

			return;
		}
	}

	TEST_FAIL_MESSAGE("There doesn't seem to be a message line.");
}

void check_delete_pin(const char *delete_page, const uint8_t pin_nr,
		const char *name, const bool pull_up, const bool state,
		const uint64_t changes) {
	// Split page into its lines.
	const std::unique_ptr<std::vector<std::string>> lines = split_lines(
			delete_page);

	// Find the starting line of the pin block.
	size_t i = 0;
	for (; i < lines->size(); i++) {
		if ((*lines)[i] == "<form method=\"post\" action=\"/settings.html\">") {
			break;
		}
	}

	TEST_ASSERT_LESS_THAN_MESSAGE(lines->size(), i + 20,
			"After the beginning of the pin block there were not enough lines left, or no pin block was found.");

	// Make sure the output lines are generated correctly.
	check_index_pin_line(&(*lines)[++i], "Name", name);
	check_index_pin_line(&(*lines)[++i], "Pin", (uint64_t) pin_nr);
	check_index_pin_line(&(*lines)[++i], "Resistor",
			pull_up ? "Pull Up" : "Pull Down");
	check_index_pin_line(&(*lines)[++i], "State", state ? "High" : "Low");
	check_index_pin_line(&(*lines)[++i], "Changes", changes);

	// Make sure the value lines of the hidden inputs are generated correctly.
	std::unique_ptr<std::ostringstream> str(new std::ostringstream);
	*str << "<input type=\"text\" name=\"name\" value=\"";
	*str << name;
	*str << '"';
	TEST_ASSERT_EQUAL_STRING_MESSAGE(str->str().c_str(), (*lines)[++i].c_str(),
			"The hidden name input line wasn't generated correctly.");
	*str = std::ostringstream();
	*str << "value=\"";
	*str << (uint16_t) pin_nr;
	*str << "\" max=\"39\" min=\"0\" title=\"The hardware pin to watch.\"";
	TEST_ASSERT_EQUAL_STRING_MESSAGE(str->str().c_str(),
			(*lines)[i += 6].c_str(),
			"The hidden pin input line wasn't generated correctly.");
	*str = std::ostringstream();
	*str << "<input type=\"text\" name=\"resistor\" value=\"";
	*str << (pull_up ? "Pull Up" : "Pull Down");
	*str << '"';
	TEST_ASSERT_EQUAL_STRING_MESSAGE(str->str().c_str(),
			(*lines)[i += 3].c_str(),
			"The hidden resistor input line wasn't generated correctly.");
}

void init_web_server() {
	WiFi.disconnect(1);

	WiFi.mode(WIFI_AP);

	WiFi.softAP("ESP WiFi GPIO Monitor Test", "This password doesn't matter.");

	MDNS.begin("esp-test");

	web_server.setup();
}

void run_webserver_tests() {
	init_web_server();

	RUN_TEST(test_static_pages);
	RUN_TEST(test_metrics_endpoint);
	RUN_TEST(test_pins_json);
	RUN_TEST(test_index_html);
	RUN_TEST(test_settings_html);
	RUN_TEST(test_delete_html);

	WiFi.disconnect(1, 1);
}

void test_static_pages() {
	// Make sure main.css generates status code 200 and content type text/css.
	client.begin("http://localhost/main.css");
	const char *headerkeys[] = { "Content-Type" };
	const size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
	client.collectHeaders(headerkeys, headerkeyssize);
	TEST_ASSERT_EQUAL_MESSAGE(200, client.GET(),
			"Get /main.css did not return http status code 200.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("text/css",
			client.header("Content-Type").c_str(),
			"Get /main.css did not return content type text/css.");
	TEST_ASSERT_GREATER_THAN_MESSAGE(0, client.getString().length(),
			"Get /main.css returned an empty page.");
	client.end();

	// Make sure index.js returns status code 200 and content type text/javascript.
	client.begin("http://localhost/index.js");
	TEST_ASSERT_EQUAL_MESSAGE(200, client.GET(),
			"Get /index.js did not return http status code 200.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("text/javascript",
			client.header("Content-Type").c_str(),
			"Get /index.js did not return content type text/css.");
	TEST_ASSERT_GREATER_THAN_MESSAGE(0, client.getString().length(),
			"Get /index.js returned an empty page.");
	client.end();

	// Make sure settings.js returns status code 200 and content type text/javascript.
	client.begin("http://localhost/settings.js");
	TEST_ASSERT_EQUAL_MESSAGE(200, client.GET(),
			"Get /settings.js did not return http status code 200.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("text/javascript",
			client.header("Content-Type").c_str(),
			"Get /settings.js did not return content type text/css.");
	TEST_ASSERT_GREATER_THAN_MESSAGE(0, client.getString().length(),
			"Get /settings.js returned an empty page.");
	client.end();
}

void test_metrics_endpoint() {
	// Init http client and output pin.
	const char *url = "http://localhost/metrics";
	client.begin(url);
	const char *headerkeys[] = { "Content-Type" };
	const size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
	client.collectHeaders(headerkeys, headerkeyssize);
	pinMode(OUT_PIN, OUTPUT);

	// Make sure the web server returns status 200 OK and content type text/plain.
	TEST_ASSERT_EQUAL_MESSAGE(200, client.GET(),
			"Metrics endpoint didn't return http status 200 OK.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("text/plain",
			client.header("Content-Type").c_str(),
			"The metrics endpoint did not return content type text/plain.");

	// Make sure the metrics endpoint doesn't return any data if there is no registered pin.
	TEST_ASSERT_EQUAL_MESSAGE(0, client.getSize(),
			"Metrics endpoint did not return an empty string.");

	// Make sure that the metrics endpoint returns data after registering a pin to watch.
	digitalWrite(OUT_PIN, LOW);
	gpio_handler.registerGPIO(IN_PIN, "Test", false);
	client.end();
	client.begin(url);
	TEST_ASSERT_EQUAL_MESSAGE(200, client.GET(),
			"Metrics endpoint didn't return stats 200 after registering a pin.");
	TEST_ASSERT_GREATER_THAN_MESSAGE(0, client.getSize(),
			"Metrics endpoint did not return data after registering a pin.");

	// Test whether the HELP and TYPE lines for the current state metrics are generated correctly.
	std::unique_ptr<String> metric_page(new String(client.getString()));
	client.end();
	check_pin_metric(metric_page->c_str(), "esp_pin_state", IN_PIN, "Test",
			"gauge", 0);
	check_pin_metric(metric_page->c_str(), "esp_pin_state_changes", IN_PIN,
			"Test", "counter", 0);

	// Toggle gpio state a few time and test changes counter.
	gpio_handler.setDebounceTimeout(2);
	bool state = false;
	for (uint8_t i = 0; i < 19; i++) {
		digitalWrite(OUT_PIN, state ? LOW : HIGH);
		state = !state;
		delay(3);
	}
	client.begin(url);
	client.GET();
	*metric_page = client.getString();
	client.end();
	check_pin_metric(metric_page->c_str(), "esp_pin_state", IN_PIN, "Test",
			"gauge", 1);
	check_pin_metric(metric_page->c_str(), "esp_pin_state_changes", IN_PIN,
			"Test", "counter", 19);

	// Test having two registered pins.
	gpio_handler.registerGPIO(IN_PIN_2, "Some other test pin", true);
	client.begin(url);
	client.GET();
	*metric_page = client.getString();
	client.end();
	check_pin_metric(metric_page->c_str(), "esp_pin_state", IN_PIN, "Test",
			"gauge", 1);
	check_pin_metric(metric_page->c_str(), "esp_pin_state_changes", IN_PIN,
			"Test", "counter", 19);
	check_pin_metric(metric_page->c_str(), "esp_pin_state", IN_PIN_2,
			"Some other test pin", "gauge", 1);
	check_pin_metric(metric_page->c_str(), "esp_pin_state_changes", IN_PIN_2,
			"Some other test pin", "counter", 0);

	// Unregister pins from the gpiohandler.
	gpio_handler.unregisterGPIO(IN_PIN_2);
	gpio_handler.unregisterGPIO(IN_PIN);
}

void test_pins_json() {
	// Make sure pins.json returns status code 200 and content type application/json
	const char *url = "http://localhost/pins.json";
	client.begin(url);
	const char *headerkeys[] = { "Content-Type" };
	const size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
	client.collectHeaders(headerkeys, headerkeyssize);
	TEST_ASSERT_EQUAL_MESSAGE(200, client.GET(),
			"A get request to /pins.json did not return status code 200 OK.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("application/json",
			client.header("Content-Type").c_str(),
			"pins.json did not return the correct content-type.");

	// Make sure the json is an empty array of no pins are registered.
	TEST_ASSERT_EQUAL_STRING_MESSAGE("{}",
			client.getString().substring(0, 2).c_str(),
			"pins.json did not return an empty json object while no pins were registered.");
	client.end();

	// Test whether a registered pin generates a correct json object.
	pinMode(OUT_PIN, OUTPUT);
	digitalWrite(OUT_PIN, HIGH);
	gpio_handler.registerGPIO(IN_PIN, "Test Pin", true);
	client.begin(url);
	client.GET();
	std::unique_ptr<String> pins_json(new String(client.getString()));
	client.end();
	check_json_pin(pins_json->c_str(), IN_PIN, "Test Pin", true, true, 0);

	// Test pins json after a state change.
	digitalWrite(OUT_PIN, LOW);
	client.begin(url);
	client.GET();
	*pins_json = client.getString();
	client.end();
	check_json_pin(pins_json->c_str(), IN_PIN, "Test Pin", true, false, 1);

	// Test pins json with two registered pins.
	gpio_handler.registerGPIO(IN_PIN_2, "Pin2", false);
	digitalWrite(OUT_PIN, HIGH);
	client.begin(url);
	client.GET();
	*pins_json = client.getString();
	client.end();
	check_json_pin(pins_json->c_str(), IN_PIN, "Test Pin", true, true, 2);
	check_json_pin(pins_json->c_str(), IN_PIN_2, "Pin2", false, false, 0);

	// Unregister pins from the gpiohandler.
	gpio_handler.unregisterGPIO(IN_PIN_2);
	gpio_handler.unregisterGPIO(IN_PIN);
}

void test_index_html() {
	// Make sure pins aren't still registered from failed tests.
	gpio_handler.unregisterGPIO(IN_PIN_2);
	gpio_handler.unregisterGPIO(IN_PIN);

	// Make sure /index.html returns content type text/html and status code 200.
	const char *url = "http://localhost/index.html";
	client.begin(url);
	const char *headerkeys[] = { "Content-Type" };
	const size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
	client.collectHeaders(headerkeys, headerkeyssize);
	TEST_ASSERT_EQUAL_MESSAGE(200, client.GET(),
			"Get /index.html did not return status code 200.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("text/html",
			client.header("Content-Type").c_str(),
			"Get /index.html did not return content type text/html.");

	// Make sure /index.html without a registered pin doesn't return an empty page.
	// Also make sure the page contains the no registered pin message.
	std::unique_ptr<String> index_page(new String(client.getString()));
	client.end();
	TEST_ASSERT_GREATER_THAN_MESSAGE(0, index_page->length(),
			"The page returned by a get request to /index.html is empty.");
	TEST_ASSERT_NOT_EQUAL_MESSAGE(std::string::npos,
			std::string(index_page->c_str()).find(
					"Currently no pin is registered to be watched."),
			"Index page without a registered pin doesn't contain the no registered pin message.");

	// Test whether / returns status code 200 and content type text/html.
	client.begin("http://localhost/");
	TEST_ASSERT_EQUAL_MESSAGE(200, client.GET(),
			"Get /index.html did not return status code 200.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("text/html",
			client.header("Content-Type").c_str(),
			"Get /index.html did not return content type text/html.");

	// Test whether / and /index.html return the same content.
	TEST_ASSERT_EQUAL_STRING_MESSAGE(index_page->c_str(),
			client.getString().c_str(),
			"Get / and Get /index.html did not return the same content.");
	client.end();

	// Check generated pin block.
	pinMode(OUT_PIN, OUTPUT);
	digitalWrite(OUT_PIN, LOW);
	gpio_handler.registerGPIO(IN_PIN, "Test Pin", false);
	client.begin(url);
	TEST_ASSERT_EQUAL_MESSAGE(200, client.GET(),
			"Get /index.html did not return status code 200 with a registered pin.");
	*index_page = client.getString();
	client.end();
	check_index_pin(index_page->c_str(), IN_PIN, "Test Pin", false, false, 0);

	// Test pin block after a pin state change.
	digitalWrite(OUT_PIN, HIGH);
	client.begin(url);
	client.GET();
	*index_page = client.getString();
	client.end();
	check_index_pin(index_page->c_str(), IN_PIN, "Test Pin", false, true, 1);

	// Test pin block after a gpiohandler update.
	gpio_handler.updateGPIO(IN_PIN, "Test Pin Name", true);
	client.begin(url);
	client.GET();
	*index_page = client.getString();
	client.end();
	check_index_pin(index_page->c_str(), IN_PIN, "Test Pin Name", true, true,
			1);

	// Test two pin blocks.
	digitalWrite(OUT_PIN, LOW);
	gpio_handler.registerGPIO(IN_PIN_2, "Some other test pin", true);
	client.begin(url);
	client.GET();
	*index_page = client.getString();
	client.end();
	check_index_pin(index_page->c_str(), IN_PIN, "Test Pin Name", true, false,
			2);
	check_index_pin(index_page->c_str(), IN_PIN_2, "Some other test pin", true,
			true, 0);

	// Unregister pins from the gpiohandler.
	gpio_handler.unregisterGPIO(IN_PIN_2);
	gpio_handler.unregisterGPIO(IN_PIN);
}

void test_settings_html() {
	// Make sure pins aren't still registered from failed tests.
	gpio_handler.unregisterGPIO(IN_PIN_2);
	gpio_handler.unregisterGPIO(IN_PIN);

	// Make sure /settings.html returns status code 200 and content type text/html.
	const char *url = "http://localhost/settings.html";
	client.begin(url);
	const char *headerkeys[] = { "Content-Type" };
	const size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
	client.collectHeaders(headerkeys, headerkeyssize);
	TEST_ASSERT_EQUAL_MESSAGE(200, client.GET(),
			"A get request to /settings.html did not return status code 200.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("text/html",
			client.header("Content-Type").c_str(),
			"Get /settings.html did not return content type text/html.");

	// Make sure /settings.html without a registered pin doesn't return an empty page.
	// Also make sure the page contains the no registered pin message.
	std::unique_ptr<String> settings_page(new String(client.getString()));
	client.end();
	TEST_ASSERT_GREATER_THAN_MESSAGE(0, settings_page->length(),
			"The page returned by a get request to /settings.html is empty.");
	TEST_ASSERT_NOT_EQUAL_MESSAGE(std::string::npos,
			std::string(settings_page->c_str()).find(
					"Currently no pin is registered to be watched."),
			"Settings page without a registered pin doesn't contain the no registered pin message.");

	// Test generated pin blocks.
	pinMode(OUT_PIN, OUTPUT);
	digitalWrite(OUT_PIN, LOW);
	gpio_handler.registerGPIO(IN_PIN, "Test Pin", false);
	client.begin(url);
	TEST_ASSERT_EQUAL_MESSAGE(200, client.GET(),
			"Get /settings.html did not return status code 200 with a registered pin.");
	*settings_page = client.getString();
	client.end();
	check_settings_pin(settings_page->c_str(), IN_PIN, "Test Pin", false, false,
			0);

	// Test changed pin block.
	digitalWrite(OUT_PIN, HIGH);
	client.begin(url);
	client.GET();
	*settings_page = client.getString();
	client.end();
	check_settings_pin(settings_page->c_str(), IN_PIN, "Test Pin", false, true,
			1);

	// Test pin block after a gpiohandler update.
	gpio_handler.updateGPIO(IN_PIN, "Test Pin Name", true);
	client.begin(url);
	client.GET();
	*settings_page = client.getString();
	client.end();
	check_settings_pin(settings_page->c_str(), IN_PIN, "Test Pin Name", true,
			true, 1);

	// Test two pin blocks.
	digitalWrite(OUT_PIN, LOW);
	gpio_handler.registerGPIO(IN_PIN_2, "Some other test pin", true);
	client.begin(url);
	client.GET();
	*settings_page = client.getString();
	client.end();
	check_settings_pin(settings_page->c_str(), IN_PIN, "Test Pin Name", true,
			false, 2);
	check_settings_pin(settings_page->c_str(), IN_PIN_2, "Some other test pin",
			true, true, 0);

	// Make sure there isn't a message by default.
	check_message(settings_page->c_str(), "$message_type", "$message", true);

	// Test registering a pin using a post request.
	gpio_handler.unregisterGPIO(IN_PIN_2);
	client.begin(url);
	client.addHeader("Content-Type", "application/x-www-form-urlencoded");
	std::unique_ptr<std::ostringstream> post_body(new std::ostringstream());
	*post_body << "name=Test&pin=";
	*post_body << (uint16_t) IN_PIN_2;
	*post_body << "&resistor=pull_down&action=add";
	TEST_ASSERT_EQUAL_MESSAGE(200, client.POST(post_body->str().c_str()),
			"POST /settings.html registering a pin didn't return status code 200.");
	*settings_page = client.getString();
	client.end();
	check_settings_pin(settings_page->c_str(), IN_PIN, "Test Pin Name", true,
			false, 2);
	check_settings_pin(settings_page->c_str(), IN_PIN_2, "Test", false, false,
			0);
	check_message(settings_page->c_str(), "success",
			"Successfully added Pin to be watched.", false);

	// Test registering an already registered pin.
	client.begin(url);
	client.addHeader("Content-Type", "application/x-www-form-urlencoded");
	TEST_ASSERT_EQUAL_MESSAGE(400, client.POST(post_body->str().c_str()),
			"POST /settings.html registering an already registered pin didn't return status code 400.");
	*settings_page = client.getString();
	client.end();
	check_settings_pin(settings_page->c_str(), IN_PIN, "Test Pin Name", true,
			false, 2);
	check_settings_pin(settings_page->c_str(), IN_PIN_2, "Test", false, false,
			0);
	std::unique_ptr<std::ostringstream> str(new std::ostringstream());
	*str << "Couldn't add pin because pin ";
	*str << (uint16_t) IN_PIN_2;
	*str << " is already being watched.";
	check_message(settings_page->c_str(), "error", str->str().c_str(), false);

	// Test registering an invalid pin.
	client.begin(url);
	client.addHeader("Content-Type", "application/x-www-form-urlencoded");
	*post_body = std::ostringstream();
	*post_body << "name=Test+Pin&pin=";
	*post_body << 24;
	*post_body << "&resistor=pull_down&action=add";
	TEST_ASSERT_EQUAL_MESSAGE(400, client.POST(post_body->str().c_str()),
			"POST /settings.html registering an invalid pin didn't return status code 400.");
	*settings_page = client.getString();
	client.end();
	check_settings_pin(settings_page->c_str(), IN_PIN, "Test Pin Name", true,
			false, 2);
	check_settings_pin(settings_page->c_str(), IN_PIN_2, "Test", false, false,
			0);
	*str = std::ostringstream();
	*str << "Couldn't add pin because pin ";
	*str << 24;
	*str << " isn't an input pin.";
	check_message(settings_page->c_str(), "error", str->str().c_str(), false);

	// Test updating a pin.
	client.begin(url);
	client.addHeader("Content-Type", "application/x-www-form-urlencoded");
	*post_body = std::ostringstream();
	*post_body << "name=Test+Pin&pin=";
	*post_body << (uint16_t) IN_PIN_2;
	*post_body << "&resistor=pull_up&action=update";
	TEST_ASSERT_EQUAL_MESSAGE(200, client.POST(post_body->str().c_str()),
			"POST /settings.html updating a pin didn't return status code 200.");
	*settings_page = client.getString();
	client.end();
	check_settings_pin(settings_page->c_str(), IN_PIN, "Test Pin Name", true,
			false, 2);
	check_settings_pin(settings_page->c_str(), IN_PIN_2, "Test Pin", true, true,
			1);
	check_message(settings_page->c_str(), "success",
			"Successfully updated watched Pin.", false);

	// Test updating a not registered pin.
	client.begin(url);
	client.addHeader("Content-Type", "application/x-www-form-urlencoded");
	*post_body = std::ostringstream();
	*post_body << "name=Test+Pin&pin=";
	*post_body << (uint16_t) OUT_PIN;
	*post_body << "&resistor=pull_down&action=update";
	TEST_ASSERT_EQUAL_MESSAGE(400, client.POST(post_body->str().c_str()),
			"POST /settings.html updating a not registered pin didn't return status code 400.");
	*settings_page = client.getString();
	client.end();
	check_settings_pin(settings_page->c_str(), IN_PIN, "Test Pin Name", true,
			false, 2);
	check_settings_pin(settings_page->c_str(), IN_PIN_2, "Test Pin", true, true,
			1);
	*str = std::ostringstream();
	*str << "Couldn't update pin because pin ";
	*str << (uint16_t) OUT_PIN;
	*str << " isn't being watched.";
	check_message(settings_page->c_str(), "error", str->str().c_str(), false);

	// Test removing a registered pin.
	client.begin(url);
	client.addHeader("Content-Type", "application/x-www-form-urlencoded");
	*post_body = std::ostringstream();
	*post_body << "name=Test+Pin&pin=";
	*post_body << (uint16_t) IN_PIN_2;
	*post_body << "&resistor=pull_down&action=delete";
	TEST_ASSERT_EQUAL_MESSAGE(200, client.POST(post_body->str().c_str()),
			"POST /settings.html removing a registered pin didn't return status code 200.");
	*settings_page = client.getString();
	client.end();
	check_settings_pin(settings_page->c_str(), IN_PIN, "Test Pin Name", true,
			false, 2);
	check_message(settings_page->c_str(), "success",
			"Successfully removed watched Pin.", false);

	// Test removing a not registered pin.
	client.begin(url);
	client.addHeader("Content-Type", "application/x-www-form-urlencoded");
	*post_body = std::ostringstream();
	*post_body << "name=Test+Pin&pin=";
	*post_body << (uint16_t) IN_PIN_2;
	*post_body << "&resistor=pull_down&action=delete";
	TEST_ASSERT_EQUAL_MESSAGE(400, client.POST(post_body->str().c_str()),
			"POST /settings.html removing a not registered pin didn't return status code 400.");
	*settings_page = client.getString();
	client.end();
	check_settings_pin(settings_page->c_str(), IN_PIN, "Test Pin Name", true,
			false, 2);
	*str = std::ostringstream();
	*str << "Couldn't delete pin because pin ";
	*str << (uint16_t) IN_PIN_2;
	*str << " isn't being watched.";
	check_message(settings_page->c_str(), "error", str->str().c_str(), false);

	// Unregister pins from the gpiohandler.
	gpio_handler.unregisterGPIO(IN_PIN_2);
	gpio_handler.unregisterGPIO(IN_PIN);
}

void test_delete_html() {
	// Make sure pins aren't still registered from failed tests.
	gpio_handler.unregisterGPIO(IN_PIN_2);
	gpio_handler.unregisterGPIO(IN_PIN);

	// Make sure /delete.html returns a 404 page on GET.
	const char *url = "http://localhost/delete.html";
	client.begin(url);
	const char *headerkeys[] = { "Content-Type" };
	const size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
	client.collectHeaders(headerkeys, headerkeyssize);
	TEST_ASSERT_EQUAL_MESSAGE(404, client.GET(),
			"A get request to /delete.html did not return status code 404.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("text/html",
			client.header("Content-Type").c_str(),
			"Get /delete.html did not return content type text/html.");
	std::unique_ptr<String> delete_page(new String(client.getString()));
	client.end();
	TEST_ASSERT_EQUAL_STRING_MESSAGE(NOT_FOUND_HTML, delete_page->c_str(),
			"The response for a get request to /delete.html did not match the 404 not found page.");

	// Test trying to delete a pin using /delete.html.
	pinMode(OUT_PIN, OUTPUT);
	digitalWrite(OUT_PIN, LOW);
	gpio_handler.registerGPIO(IN_PIN, "Test Pin", true);
	client.begin(url);
	client.addHeader("Content-Type", "application/x-www-form-urlencoded");
	std::unique_ptr<std::ostringstream> post_body(new std::ostringstream());
	*post_body << "name=Test+Pin&pin=";
	*post_body << (uint16_t) IN_PIN;
	*post_body << "&resistor=pull_up";
	TEST_ASSERT_EQUAL_MESSAGE(200, client.POST(post_body->str().c_str()),
			"POST /delete.html removing a registered pin didn't return status code 200.");
	*delete_page = client.getString();
	client.end();
	check_delete_pin(delete_page->c_str(), IN_PIN, "Test Pin", true, false, 0);
	check_message(delete_page->c_str(), "error", "$message", true);

	// Test all the minor changes at once.
	// Test a pin that changed its name, its resistor, its state, and its changes count.
	// Also send a delete action parameter this time.
	digitalWrite(OUT_PIN, HIGH);
	gpio_handler.updateGPIO(IN_PIN, "Test", false);
	client.begin(url);
	client.addHeader("Content-Type", "application/x-www-form-urlencoded");
	*post_body = std::ostringstream();
	*post_body << "name=Test&pin=";
	*post_body << (uint16_t) IN_PIN;
	*post_body << "&resistor=pull_down&action=delete";
	client.POST(post_body->str().c_str());
	*delete_page = client.getString();
	client.end();
	check_delete_pin(delete_page->c_str(), IN_PIN, "Test", false, true, 1);
	check_message(delete_page->c_str(), "error", "$message", true);

	// Test trying to delete an unregistered pin using /delete.html.
	client.begin(url);
	client.addHeader("Content-Type", "application/x-www-form-urlencoded");
	*post_body = std::ostringstream();
	*post_body << "name=Test+Pin&pin=";
	*post_body << (uint16_t) IN_PIN_2;
	*post_body << "&resistor=pull_up";
	TEST_ASSERT_EQUAL_MESSAGE(400, client.POST(post_body->str().c_str()),
			"POST /delete.html removing an unregistered pin didn't return status code 400.");
	*delete_page = client.getString();
	client.end();
	check_delete_pin(delete_page->c_str(), IN_PIN_2, "", false, false, 0);
	std::unique_ptr<std::ostringstream> str(new std::ostringstream());
	*str << "Pin ";
	*str << (uint16_t) IN_PIN_2;
	*str << " isn't being watched.";
	check_message(delete_page->c_str(), "error", str->str().c_str(), false);

	// Test trying to delete an invalid pin using /delete.html.
	client.begin(url);
	client.addHeader("Content-Type", "application/x-www-form-urlencoded");
	*post_body = std::ostringstream();
	*post_body << "name=Test+Pin&pin=";
	*post_body << 24;
	*post_body << "&resistor=pull_up";
	TEST_ASSERT_EQUAL_MESSAGE(400, client.POST(post_body->str().c_str()),
			"POST /delete.html removing an invalid pin didn't return status code 400.");
	*delete_page = client.getString();
	client.end();
	check_delete_pin(delete_page->c_str(), 24, "", false, false, 0);
	*str = std::ostringstream();
	*str << "Pin ";
	*str << 24;
	*str << " isn't an input pin.";
	check_message(delete_page->c_str(), "error", str->str().c_str(), false);

	// Unregister pin from gpiohandler.
	gpio_handler.unregisterGPIO(IN_PIN);
}
