/*
 * WebServerHandler.cpp
 *
 *  Created on: 11.03.2022
 *      Author: ToMe25
 */

#include "WebServerHandler.h"
#include "GPIOHandler.h"
#include <ESPmDNS.h>
#include <functional>
#include <regex>

WebServerHandler::WebServerHandler(const uint16_t port) :
		_port(port), server(port) {
}

WebServerHandler::~WebServerHandler() {
}

void WebServerHandler::setup() {
	using namespace std::placeholders;

	server.rewrite("/", "/index.html");

	server.on("/index.html", HTTP_GET,
			std::bind(&WebServerHandler::getIndex, this, _1));

	server.on("/settings.html", HTTP_GET | HTTP_POST,
			std::bind(&WebServerHandler::handleSettings, this, _1));

	server.on("/delete.html", HTTP_POST,
			std::bind(&WebServerHandler::postDelete, this, _1));

	server.on("/pins.json", HTTP_GET,
			std::bind(&WebServerHandler::getPinsJson, this, _1));

	server.on("/metrics", HTTP_GET,
			std::bind(&WebServerHandler::getMetrics, this, _1));

	server.on("/main.css", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send_P(200, "text/css", MAIN_CSS);
	});

	server.on("/index.js", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send_P(200, "text/javascript", INDEX_JS);
	});

	server.on("/settings.js", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send_P(200, "text/javascript", SETTINGS_JS);
	});

	server.onNotFound(std::bind(&WebServerHandler::onNotFound, this, _1));

	server.begin();

	MDNS.addService("http", "tcp", _port);
}

void WebServerHandler::getMetrics(AsyncWebServerRequest *request) const {
	std::ostringstream stream;

	std::vector<pin_state> states = gpio_handler.getWatchedPins();
	if (states.size() > 0) {
		stream	<< "# HELP esp_pin_state The current digital state of an ESP GPIO/GPI pin."
				<< std::endl;
		stream << "# TYPE esp_pin_state gauge" << std::endl;

		for (pin_state state : states) {
			stream << "esp_pin_state{pin=\"" << (uint16_t) state.number
					<< "\",name=\"" << state.name.c_str() << "\"} "
					<< state.state << std::endl;
		}

		stream	<< "# HELP esp_pin_state_changes The number of times the state of the ESP GPIO/GPI pin changed its state."
				<< std::endl;
		stream << "# TYPE esp_pin_state_changes counter" << std::endl;

		for (pin_state state : states) {
			stream << "esp_pin_state_changes{pin=\"" << (uint16_t) state.number
					<< "\",name=\"" << state.name.c_str() << "\"} "
					<< state.changes << std::endl;
		}
	}

	AsyncWebServerResponse *response = request->beginResponse(200, "text/plain",
			stream.str().c_str());
	response->addHeader("Cache-Control", "no-cache");
	request->send(response);
}

void WebServerHandler::onNotFound(AsyncWebServerRequest *request) const {
	request->send_P(404, "text/html", NOT_FOUND_HTML);
}

void WebServerHandler::getIndex(AsyncWebServerRequest *request) const {
	std::string response(INDEX_HTML);

	std::vector<pin_state> watched_pins = gpio_handler.getWatchedPins();
	if (watched_pins.size() > 0) {
		std::ostringstream converter;
		std::string state_html;
		std::regex pin("\\$pin");
		std::regex name("\\$name");
		std::regex pull_up("\\$pull_up");
		std::regex state("\\$state");
		std::regex changes("\\$changes");
		std::regex end("[ \\t]*<!-- Pin states end -->");
		for (pin_state &watched : watched_pins) {
			state_html = INDEX_STATE_HTML;
			converter << (uint16_t) watched.number;
			state_html = std::regex_replace(state_html, pin, converter.str());
			converter.str("");
			converter.clear();
			state_html = std::regex_replace(state_html, name,
					watched.name.c_str());
			state_html = std::regex_replace(state_html, pull_up,
					watched.pull_up ? "Pull Up" : "Pull Down");
			state_html = std::regex_replace(state_html, state,
					watched.state ? "High" : "Low");
			converter << watched.changes;
			state_html = std::regex_replace(state_html, changes,
					converter.str());
			converter.str("");
			converter.clear();
			state_html += "<!-- Pin states end -->";
			response = std::regex_replace(response, end, state_html);
		}
	} else {
		response.insert(response.find("<!-- Pin states end -->"),
				"Currently no pin is registered to be watched.\n\t\t");
	}

	request->send(200, "text/html", response.c_str());
}

void WebServerHandler::handleSettings(AsyncWebServerRequest *request) const {
	std::string response(SETTINGS_HTML);

	bool error = false;
	if (request->method() == HTTP_POST) {
		String message = "";
		String action, pin, name;
		bool pull_up = false;

		if (!request->hasParam("action", true)) {
			message = "Received a request missing the \"action\" parameter.";
			error = true;
		} else if (!request->hasParam("pin", true)) {
			message = "Received a request missing the \"pin\" parameter.";
			error = true;
		} else if (!request->hasParam("name", true)) {
			message = "Received a request missing the \"name\" parameter.";
			error = true;
		} else if (!request->hasParam("resistor", true)) {
			message = "Received a request missing the \"resistor\" parameter.";
			error = true;
		} else {
			action = request->getParam("action", true)->value();
			name = request->getParam("name", true)->value();
			pin = request->getParam("pin", true)->value();
			const String resistor =
					request->getParam("resistor", true)->value();

			if (resistor == "pull_up" || resistor == "Pull Up") {
				pull_up = true;
			} else if (resistor != "pull_down" && resistor != "Pull Down") {
				message = "Received invalid resistor value \"" + resistor
						+ "\".";
				error = true;
			}
		}

		if (!error) {
			gpio_err_t err = GPIO_OK;
			if (action == "add") {
				err = gpio_handler.registerGPIO(atoi(pin.c_str()), name,
						pull_up);
				message = "Successfully added Pin to be watched.";
			} else if (action == "update") {
				err = gpio_handler.updateGPIO(atoi(pin.c_str()), name, pull_up);
				delay(gpio_handler.getDebounceTimeout());
				message = "Successfully updated watched Pin.";
			} else if (action == "delete") {
				err = gpio_handler.unregisterGPIO(atoi(pin.c_str()));
				message = "Successfully removed watched Pin.";
			} else if (action != "cancel") {
				message = "Received invalid action \"" + action + "\".";
				error = true;
			}

			if (err != GPIO_OK) {
				message = "Couldn't " + action + " pin because ";
				error = true;

				switch (err) {
				case GPIO_PIN_INVALID:
					message += "pin " + pin + " isn't an input pin.";
					break;
				case GPIO_NAME_INVALID:
					message += '"' + name + "\" isn't a valid pin name.";
					break;
				case GPIO_ALREADY_WATCHED:
					message += "pin " + pin + " is already being watched.";
					break;
				case GPIO_NOT_WATCHED:
					message += "pin " + pin + " isn't being watched.";
					break;
				default:
					message += "of an unknown error.";
					Serial.print(
							"WebServerHandler: A GPIOHandler action returned unknown error code ");
					Serial.print(err);
					Serial.println('.');
					break;
				}
			}
		}

		if (message != "") {
			response = std::regex_replace(response,
					std::regex("\\$message_type\" hidden"),
					error ? "error\"" : "success\"");
			response = std::regex_replace(response, std::regex("\\$message"),
					message.c_str());
		}
	}

	std::vector<pin_state> watched_pins = gpio_handler.getWatchedPins();
	if (watched_pins.size() > 0) {
		std::ostringstream converter;
		std::string pin_html;
		std::regex pin("\\$pin");
		std::regex name("\\$name");
		std::regex pull_up_checked("\\$puc");
		std::regex pull_down_checked("\\$pdc");
		std::regex state("\\$state");
		std::regex changes("\\$changes");
		std::regex end("[ \\t]*<!-- Pin states end -->");
		for (pin_state watched : watched_pins) {
			pin_html = SETTINGS_PIN_HTML;
			converter << (uint16_t) watched.number;
			pin_html = std::regex_replace(pin_html, pin, converter.str());
			converter.str("");
			converter.clear();
			pin_html = std::regex_replace(pin_html, name, watched.name.c_str());
			pin_html = std::regex_replace(pin_html, pull_up_checked,
					watched.pull_up ? "checked" : "");
			pin_html = std::regex_replace(pin_html, pull_down_checked,
					watched.pull_up ? "" : "checked");
			pin_html = std::regex_replace(pin_html, state,
					watched.state ? "High" : "Low");
			converter << watched.changes;
			pin_html = std::regex_replace(pin_html, changes, converter.str());
			converter.str("");
			converter.clear();
			pin_html += "<!-- Pin states end -->";
			response = std::regex_replace(response, end, pin_html);
		}
	} else {
		response.insert(response.find("<!-- Pin states end -->"),
				"Currently no pin is registered to be watched.\n\t\t");
	}

	request->send(error ? 400 : 200, "text/html", response.c_str());
}

void WebServerHandler::postDelete(AsyncWebServerRequest *request) const {
	std::string response = DELETE_HTML;

	pin_state pin(NULL, 0, "", false, false);
	String error;
	if (request->hasParam("pin", true)) {
		uint8_t pin_nr = atoi(request->getParam("pin", true)->value().c_str());
		if (gpio_handler.isWatched(pin_nr)) {
			for (pin_state pin_state : gpio_handler.getWatchedPins()) {
				if (pin_state.number == pin_nr) {
					pin = pin_state;
				}
			}
		} else {
			error = "Pin ";
			error += (uint16_t) pin_nr;
			if (digitalPinIsValid(pin_nr)) {
				error += " isn't being watched.";
			} else {
				error += " isn't an input pin.";
			}
			pin.number = pin_nr;
		}
	} else {
		error = "Didn't receive pin to delete.";
	}

	std::ostringstream converter;
	converter << (uint16_t) pin.number;
	response = std::regex_replace(response, std::regex("\\$pin"), converter.str());
	converter.str("");
	converter.clear();
	response = std::regex_replace(response, std::regex("\\$name"), pin.name.c_str());
	response = std::regex_replace(response, std::regex("\\$pull_up"),
			pin.pull_up ? "Pull Up" : "Pull Down");
	response = std::regex_replace(response, std::regex("\\$state"),
			pin.state ? "High" : "Low");
	converter << pin.changes;
	response = std::regex_replace(response, std::regex("\\$changes"), converter.str());
	converter.str("");
	converter.clear();

	if (error.length() > 0) {
		response = std::regex_replace(response, std::regex(" hidden>\\$message"), (String(">") + error).c_str());
		response = std::regex_replace(response, std::regex("\\$confirm"), "hidden");
	} else {
		response = std::regex_replace(response, std::regex("\\$confirm"), "");
	}

	request->send(error.length() > 0 ? 400 : 200, "text/html", response.c_str());
}

void WebServerHandler::getPinsJson(AsyncWebServerRequest *request) const {
	std::ostringstream json;
	json << '{';

	bool first = true;
	for (pin_state state : gpio_handler.getWatchedPins()) {
		if (first) {
			first = false;
		} else {
			json << ',' << std::endl;
		}

		json << '"' << (uint16_t) state.number << "\": ";
		json << "{\"pin\": " << (uint16_t) state.number;
		json << ", \"name\": \"" << state.name.c_str();
		json << "\", \"pull_up\": " << (state.pull_up ? "true" : "false");
		json << ", \"state\": \"" << (state.state ? "High" : "Low");
		json << "\", \"changes\": " << state.changes;
		json << '}';
	}
	json << '}' << std::endl;

	AsyncWebServerResponse *response = request->beginResponse(200,
			"application/json", json.str().c_str());
	response->addHeader("Cache-Control", "no-cache");
	request->send(response);
}
