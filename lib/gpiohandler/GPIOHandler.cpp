/*
 * gpiohandler.cpp
 *
 *  Created on: 05.03.2022
 *      Author: ToMe25
 */

#include "GPIOHandler.h"

GPIOHandler gpio_handler;

GPIOHandler::GPIOHandler() {
	dirty_states.reserve(20);
}

GPIOHandler::~GPIOHandler() {
	if (debounce_timer != NULL) {
		esp_timer_stop(debounce_timer);
		esp_timer_delete(debounce_timer);
	}
}

gpio_err_t GPIOHandler::registerGPIO(const uint8_t pin, String name, const bool pull_up) {
	if (!digitalPinIsValid(pin)) {
		return GPIO_PIN_INVALID;
	}

	if (watched.count(pin) > 0) {
		return GPIO_ALREADY_WATCHED;
	}

	name.trim();

	if (name.length() < 3 || name.length() > 32) {
		return GPIO_NAME_INVALID;
	}

	if (!std::all_of(name.begin(), name.end(), GPIOHandler::isValidNameChar)) {
		return GPIO_NAME_INVALID;
	}

	if (debounce_timeout > 0 && debounce_timer == NULL) {
		esp_timer_create(&debounce_timer_config, &debounce_timer);
	}

	if (pull_up) {
		pinMode(pin, INPUT_PULLUP);
	} else {
		pinMode(pin, INPUT_PULLDOWN);
	}

	pin_state state(this, pin, name, pull_up, digitalRead(pin) == HIGH);

	watched.insert(std::pair<uint8_t, pin_state>(pin, state));

	if (interrupts) {
		attachInterruptArg(pin, pinInterrupt, &watched.at(pin), CHANGE);
	}

	return GPIO_OK;
}

gpio_err_t GPIOHandler::unregisterGPIO(const uint8_t pin) {
	if (watched.count(pin) == 0) {
		return GPIO_NOT_WATCHED;
	}

	dirty_states.erase(
			std::remove(dirty_states.begin(), dirty_states.end(),
					&watched.at(pin)), dirty_states.end());
	watched.erase(pin);
	detachInterrupt(pin);
	return GPIO_OK;
}

gpio_err_t GPIOHandler::updateGPIO(const uint8_t pin, String name, const bool pull_up) {
	if (watched.count(pin) == 0) {
		return GPIO_NOT_WATCHED;
	}

	name.trim();

	if (name.length() < 3 || name.length() > 32) {
		return GPIO_NAME_INVALID;
	}

	if (!std::all_of(name.begin(), name.end(), GPIOHandler::isValidNameChar)) {
		return GPIO_NAME_INVALID;
	}

	watched.at(pin).name = name;

	if (watched.at(pin).pull_up != pull_up) {
		watched.at(pin).pull_up = pull_up;

		if (pull_up) {
			pinMode(pin, INPUT_PULLUP);
		} else {
			pinMode(pin, INPUT_PULLDOWN);
		}

		updatePin(&watched.at(pin));

		if (interrupts) {
			attachInterruptArg(pin, pinInterrupt, &watched.at(pin), CHANGE);
		}
	}

	return GPIO_OK;
}

void GPIOHandler::checkPins() {
	for (std::pair<uint8_t, pin_state> entry : watched) {
		updatePin(&watched.at(entry.first));
	}
}

bool GPIOHandler::getState(const uint8_t pin) const {
	if (watched.count(pin) == 0) {
		return false;
	}

	return watched.at(pin).state;
}

uint64_t GPIOHandler::getChanges(const uint8_t pin) const {
	if (watched.count(pin) == 0) {
		return UINT64_MAX;
	}

	return watched.at(pin).changes;
}

String GPIOHandler::getName(const uint8_t pin) const {
	if (watched.count(pin) == 0) {
		return "";
	}

	return watched.at(pin).name;
}

bool GPIOHandler::isWatched(const uint8_t pin) const {
	return watched.count(pin) != 0;
}

std::vector<pin_state> GPIOHandler::getWatchedPins() const {
	std::vector<pin_state> states;
	states.reserve(watched.size());
	for (std::pair<uint8_t, pin_state> entry : watched) {
		states.push_back(entry.second);
	}
	return states;
}

void GPIOHandler::disableInterrupts() {
	interrupts = false;
	for (std::pair<uint8_t, pin_state> entry : watched) {
		detachInterrupt(entry.first);
	}
}

void GPIOHandler::enableInterrupts() {
	interrupts = true;
	for (std::pair<uint8_t, pin_state> entry : watched) {
		attachInterruptArg(entry.first, pinInterrupt, &watched.at(entry.first), CHANGE);
	}
}

bool GPIOHandler::interrupsEnabled() const {
	return interrupts;
}

void GPIOHandler::setDebounceTimeout(const uint16_t timeout) {
	debounce_timeout = timeout;

	if (timeout == 0) {
		esp_timer_stop(debounce_timer);
		for (pin_state *pin : dirty_states) {
			updatePin(pin);
		}
		dirty_states.clear();
	}
}

uint16_t GPIOHandler::getDebounceTimeout() const {
	return debounce_timeout;
}

void IRAM_ATTR GPIOHandler::pinInterrupt(void *arg) {
	if (arg == NULL) {
		return;
	}

	pin_state *pin = (pin_state *) arg;
	pin->handler->updatePin(pin);
}

void IRAM_ATTR GPIOHandler::timerInterrupt(void *arg) {
	if (arg == NULL) {
		return;
	}

	GPIOHandler *handler = (GPIOHandler *) arg;
	uint16_t next_update = 0;
	const uint64_t now = millis();
	pin_state *pin;
	for (size_t i = 0; i < handler->dirty_states.size(); i++) {
		pin = handler->dirty_states[i];
		if (pin == NULL) {
			continue;
		}

		if (now - pin->raw_last_change >= handler->debounce_timeout) {
			bool state = digitalRead(pin->number) == HIGH;
			if (state != pin->raw_state) {
				pin->raw_state = state;
				pin->raw_last_change = now;
				if (next_update == 0) {
					next_update = handler->debounce_timeout;
				}
				continue;
			}

			if (pin->state != pin->raw_state) {
				pin->state = pin->raw_state;
				pin->last_change = pin->raw_last_change;
				pin->changes++;
				handler->dirty_states[i] = NULL;
			}
		} else if (next_update == 0 || handler->debounce_timeout + pin->raw_last_change - now < next_update) {
			next_update = handler->debounce_timeout + pin->raw_last_change - now;
		}
	}

	handler->dirty_states.erase(
			std::remove(handler->dirty_states.begin(),
					handler->dirty_states.end(), (pin_state*) NULL),
			handler->dirty_states.end());

	if (next_update == 0 && handler->dirty_states.size() > 0) {
		for (pin_state *pin : handler->dirty_states) {
			if (next_update == 0 || handler->debounce_timeout + pin->raw_last_change - now < next_update) {
				next_update = handler->debounce_timeout + pin->raw_last_change - now;
				if (next_update == 0) {
					next_update = 1;
				}
			}
		}
	}

	if (next_update > 0) {
		handler->startTimer(next_update);
	}
}

void IRAM_ATTR GPIOHandler::updatePin(pin_state *pin) {
	if (pin == NULL) {
		return;
	}

	bool state = digitalRead(pin->number) == HIGH;

	if (state != pin->raw_state) {
		pin->raw_state = state;
		pin->raw_last_change = millis();

		if (debounce_timeout > 0) {
			if (std::count(dirty_states.begin(), dirty_states.end(), pin) == 0) {
				dirty_states.push_back(pin);
				pin->handler->startTimer(debounce_timeout);
			}
		} else {
			pin->state = state;
			pin->last_change = pin->raw_last_change;
			pin->changes++;
		}
	}
}

void IRAM_ATTR GPIOHandler::startTimer(const uint16_t delay) {
	if (!esp_timer_is_active(debounce_timer)) {
		esp_timer_start_once(debounce_timer, delay);
	}
}

bool GPIOHandler::isValidNameChar(const char c) {
	return isAlphaNumeric(c) || c == ' ' || c == '_' || c == '-';
}
