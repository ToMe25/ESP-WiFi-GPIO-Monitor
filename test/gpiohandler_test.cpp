/*
 * gpiohandler_test.cpp
 *
 *  Created on: 11.03.2022
 *      Author: ToMe25
 */

#include "gpiohandler_test.h"
#include "test_main.h"
#include "GPIOHandler.h"
#include <unity.h>

void bounce_pin(const uint8_t pin, const uint8_t max_bounce_time,
		const bool target, const uint8_t bounces) {
	bool valid = digitalPinIsValid(pin) && digitalPinCanOutput(pin);

	TEST_ASSERT_MESSAGE(valid,
			"The pin given to bounce_pin is not a valid GPIO pin!");

	bool state = !target; // assume the pin is not currently in the target state.
	for (uint8_t i = 0; i < bounces * 2 - 1; i++) {
		digitalWrite(pin, state ? LOW : HIGH);
		state = !state;
		if (i < (bounces - 1) * 2) {
			const uint16_t wait = rand() % (max_bounce_time * 1000) + 1;
			delayMicroseconds(wait);
		}
	}
}

void run_gpiohandler_tests() {
	RUN_TEST(test_gpiohandler_methods);
	RUN_TEST(test_pins_connected);
	RUN_TEST(test_pin_raw_state_with_interrupt);
	RUN_TEST(test_pin_raw_state_without_interrupt);
	RUN_TEST(test_pin_state_with_interrupt);
	RUN_TEST(test_pin_state_without_interrupt);
	RUN_TEST(test_pin_state_with_interrupt_without_debounce);
	RUN_TEST(test_pin_state_without_interrupt_without_debounce);
}

void test_gpiohandler_methods() {
	// Make sure there is no pin already registered.
	TEST_ASSERT_EQUAL_MESSAGE(0, gpio_handler.getWatchedPins().size(),
			"The number of registered pins was not 0 at startup.");

	// Make sure that invalid pins cannot be registered.
	TEST_ASSERT_EQUAL_MESSAGE(GPIO_PIN_INVALID,
			gpio_handler.registerGPIO(24, "Test", false),
			"Registering the non GPI pin 24 didn't return the invalid pin error.");
	TEST_ASSERT_EQUAL_MESSAGE(GPIO_PIN_INVALID,
			gpio_handler.registerGPIO(42, "Test", false),
			"Registering the invalid pin 42 didn't return the invalid pin error.");

	// Make sure that pins with invalid names cannot be registered.
	TEST_ASSERT_EQUAL_MESSAGE(GPIO_NAME_INVALID,
			gpio_handler.registerGPIO(IN_PIN, "", false),
			"Registering a pin with an empty name didn't return the invalid name error.");
	TEST_ASSERT_EQUAL_MESSAGE(GPIO_NAME_INVALID,
			gpio_handler.registerGPIO(IN_PIN, "Ab", false),
			"Registering a pin with a name that is too short didn't return the invalid name error.");
	TEST_ASSERT_EQUAL_MESSAGE(GPIO_NAME_INVALID,
			gpio_handler.registerGPIO(IN_PIN,
					"AbCdEfGhIjKlMnOpQrStUvWxYz0123456789", false),
			"Registering a pin with a name that is too long didn't return the invalid name error.");
	TEST_ASSERT_EQUAL_MESSAGE(GPIO_NAME_INVALID,
			gpio_handler.registerGPIO(IN_PIN, "Test Name\\", false),
			"Registering a pin with a name containing a backslash didn't return the invalid name error.");
	TEST_ASSERT_EQUAL_MESSAGE(GPIO_NAME_INVALID,
			gpio_handler.registerGPIO(IN_PIN, "TEST\nNAME", false),
			"Registering a pin with a name containing a line break didn't return the invalid name error.");

	// Make sure GPIO pins can be registered, and that they actually get added to the watched list.
	TEST_ASSERT_EQUAL_MESSAGE(GPIO_OK,
			gpio_handler.registerGPIO(IN_PIN, "Test Pin_Name-5", false),
			"A standard GPIO pin could not be registered.");
	TEST_ASSERT_EQUAL_MESSAGE(1, gpio_handler.getWatchedPins().size(),
			"After registering one pin the size of the watched list is not 1.");
	TEST_ASSERT_EQUAL_MESSAGE(IN_PIN, gpio_handler.getWatchedPins()[0].number,
			"The number of the registered pin did not match the one given to the register method.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("Test Pin_Name-5",
			gpio_handler.getWatchedPins()[0].name.c_str(),
			"The name of the pin_state did not match the one given to the register method.");
	TEST_ASSERT_FALSE_MESSAGE(gpio_handler.getWatchedPins()[0].pull_up,
			"The pull_up state of the registered pin did not match the value given to the register method.");

	// Test GPIOHandler#getName for the newly registered pin.
	TEST_ASSERT_EQUAL_STRING_MESSAGE(
			gpio_handler.getWatchedPins()[0].name.c_str(),
			gpio_handler.getName(IN_PIN).c_str(),
			"The result of GPIOHandler#getName didn't match the registered name.");

	// Make sure a pin can not be registered twice.
	TEST_ASSERT_EQUAL_MESSAGE(GPIO_ALREADY_WATCHED,
			gpio_handler.registerGPIO(IN_PIN, "Test", false),
			"Unregistering an already registered pin didn't return the already watched error.");

	// Test updating a not registered pin.
	TEST_ASSERT_EQUAL_MESSAGE(GPIO_NOT_WATCHED,
			gpio_handler.updateGPIO(10, "Test", true),
			"Updating a not registered pin didn't return a not watched error.");

	// Test updating a pin to an invalid name.
	TEST_ASSERT_EQUAL_MESSAGE(GPIO_NAME_INVALID,
			gpio_handler.updateGPIO(IN_PIN, "", false),
			"Updating a pin with an empty name didn't return the invalid name error.");
	TEST_ASSERT_EQUAL_MESSAGE(GPIO_NAME_INVALID,
			gpio_handler.updateGPIO(IN_PIN, "Ab", false),
			"Updating a pin with a name that is too short didn't return the invalid name error.");
	TEST_ASSERT_EQUAL_MESSAGE(GPIO_NAME_INVALID,
			gpio_handler.updateGPIO(IN_PIN,
					"AbCdEfGhIjKlMnOpQrStUvWxYz0123456789", false),
			"Updating a pin with a name that is too long didn't return the invalid name error.");
	TEST_ASSERT_EQUAL_MESSAGE(GPIO_NAME_INVALID,
			gpio_handler.updateGPIO(IN_PIN, "Test Name\\", false),
			"Updating a pin with a name containing a backslash didn't return the invalid name error.");
	TEST_ASSERT_EQUAL_MESSAGE(GPIO_NAME_INVALID,
			gpio_handler.updateGPIO(IN_PIN, "TEST\nNAME", false),
			"Updating a pin with a name containing a line break didn't return the invalid name error.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("Test Pin_Name-5",
			gpio_handler.getName(IN_PIN).c_str(),
			"Trying to update a pin with an invalid name changed its name.");

	// Test updating an already existing pin.
	TEST_ASSERT_EQUAL_MESSAGE(GPIO_OK,
			gpio_handler.updateGPIO(IN_PIN, "Pin", true),
			"Updating an already registered pin failed.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("Pin",
			gpio_handler.getName(IN_PIN).c_str(),
			"The name of the pin_state did not match the one given to the update method.");
	TEST_ASSERT_MESSAGE(gpio_handler.getWatchedPins()[0].pull_up,
			"The pull_up state of the registered pin did not match the value given to the update method.");

	// Test updating a pin to a complex name.
	TEST_ASSERT_EQUAL_MESSAGE(GPIO_OK,
			gpio_handler.updateGPIO(IN_PIN, "Pin_Test Name-45", true),
			"Updating an already registered pin with a complex name failed.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("Pin_Test Name-45",
			gpio_handler.getName(IN_PIN).c_str(),
			"The name of the pin_state did not match the one given to the update method.");

	// Test isWatched for a registered pin.
	TEST_ASSERT_MESSAGE(gpio_handler.isWatched(IN_PIN), "isWatched returned false for a registered pin.");

	// Test isWatched for a not registered pin.
	TEST_ASSERT_FALSE_MESSAGE(gpio_handler.isWatched(10), "isWatched returned true for a not registered pin.");

	// Test setting the debouncing timeout.
	gpio_handler.setDebounceTimeout(100);
	TEST_ASSERT_EQUAL_MESSAGE(100, gpio_handler.getDebounceTimeout(),
			"The debounce timeout did not match the set value.");

	// Test whether pin interrupts are enabled by default.
	TEST_ASSERT_MESSAGE(gpio_handler.interrupsEnabled(),
			"Pin interrupts aren't enabled by default.");

	// Test disabling interrupts.
	gpio_handler.disableInterrupts();
	TEST_ASSERT_FALSE_MESSAGE(gpio_handler.interrupsEnabled(),
			"Disabling pin interrupts didn't work.");

	// Test reenabling pin interrupts.
	gpio_handler.enableInterrupts();
	TEST_ASSERT_MESSAGE(gpio_handler.interrupsEnabled(),
			"Re-enabling pin interrupts didn't work.");

	// Test unregistering a not registered pin.
	TEST_ASSERT_EQUAL_MESSAGE(GPIO_NOT_WATCHED, gpio_handler.unregisterGPIO(10),
			"Unregistering a not registered pin didn't return a not watched error.");

	// Test unregistering a registered pin.
	TEST_ASSERT_EQUAL_MESSAGE(GPIO_OK, gpio_handler.unregisterGPIO(IN_PIN),
			"Unregistering a watched pin failed.");
	TEST_ASSERT_EQUAL_MESSAGE(0, gpio_handler.getWatchedPins().size(),
			"After unregistering the only watched pin the number of watched pins was not 0.");
}

void test_pins_connected() {
	pinMode(OUT_PIN, OUTPUT);
	pinMode(IN_PIN, INPUT_PULLDOWN);

	// Set the output pin to low and check whether the input pin is low as well.
	digitalWrite(OUT_PIN, LOW);
	TEST_ASSERT_EQUAL_MESSAGE(LOW, digitalRead(IN_PIN),
			"IN_PIN state did not match output pin state. Please check your wiring.");

	// Set the output pin to high and check whether the input pin is high as well.
	digitalWrite(OUT_PIN, HIGH);
	TEST_ASSERT_EQUAL_MESSAGE(HIGH, digitalRead(IN_PIN),
			"IN_PIN state did not match output pin state. Please check your wiring.");

	// Bounce the output pin around a bit and set it to low.
	bounce_pin(OUT_PIN, 8, false, rand() % 25 + 1);
	TEST_ASSERT_EQUAL_MESSAGE(LOW, digitalRead(IN_PIN),
			"IN_PIN state after bouncing was not low as it should have been.");

	// Make sure IN_PIN_2 changes when its input resistor is changed.
	pinMode(IN_PIN_2, INPUT_PULLDOWN);
	TEST_ASSERT_EQUAL_MESSAGE(LOW, digitalRead(IN_PIN_2),
			"IN_PIN_2 state with a pull down resistor was not low.");
	pinMode(IN_PIN_2, INPUT_PULLUP);
	delay(1);
	TEST_ASSERT_EQUAL_MESSAGE(HIGH, digitalRead(IN_PIN_2),
			"IN_PIN_2 state with a pull up resistor was not high.");
}

void test_pin_raw_state_with_interrupt() {
	test_pin_state(true, true, false);
}

void test_pin_raw_state_without_interrupt() {
	test_pin_state(true, false, false);
}

void test_pin_state_with_interrupt() {
	test_pin_state(false, true, true);
}

void test_pin_state_without_interrupt() {
	test_pin_state(false, false, true);
}

void test_pin_state_with_interrupt_without_debounce() {
	test_pin_state(false, true, false);
}

void test_pin_state_without_interrupt_without_debounce() {
	test_pin_state(false, false, false);
}

void test_pin_state(bool raw, bool interrupt, bool debounce) {
	// Debouncing doesn't appy to raw pin states anyways.
	if (raw) {
		debounce = false;
	}

	// Initialize the primary input and output pin.
	pinMode(OUT_PIN, OUTPUT);
	digitalWrite(OUT_PIN, LOW);

	// Initialize GPIOHandler, register input pin, and check that its initial state is detected correctly.
	gpio_handler.setDebounceTimeout(debounce ? 10 : 0);
	if (interrupt) {
		gpio_handler.enableInterrupts();
		TEST_ASSERT_MESSAGE(gpio_handler.interrupsEnabled(),
				"Pin interrupts are still disabled after enabling them.");
	} else {
		gpio_handler.disableInterrupts();
		TEST_ASSERT_FALSE_MESSAGE(gpio_handler.interrupsEnabled(),
				"Pin interrupts are still enabled after disabling them.");
	}
	gpio_handler.unregisterGPIO(IN_PIN); // make sure the pins aren't still registered from a failed test.
	gpio_handler.unregisterGPIO(IN_PIN_2);
	gpio_handler.registerGPIO(IN_PIN, "Test", true);
	uint64_t changes = 0;
	if (raw) {
		TEST_ASSERT_FALSE_MESSAGE(gpio_handler.getWatchedPins()[0].raw_state,
				"Initial input pin raw_state was not low after registering pin.");
		TEST_ASSERT_EQUAL_MESSAGE(0,
				gpio_handler.getWatchedPins()[0].raw_last_change,
				"raw_last_change was not 0 after registering input pin.");
	} else {
		TEST_ASSERT_FALSE_MESSAGE(gpio_handler.getState(IN_PIN),
				"Initial input pin state was not detected as low by GPIOHandler.")
		TEST_ASSERT_EQUAL_MESSAGE(changes, gpio_handler.getChanges(IN_PIN),
				"The input pin state change counter was not 0 after registering the pin.");
		TEST_ASSERT_EQUAL_MESSAGE(0, gpio_handler.getWatchedPins()[0].last_change,
				"last_change was not 0 after registering input pin.");
	}

	// Set output state to high and test detection.
	digitalWrite(OUT_PIN, HIGH);
	changes++;
	if (!interrupt) {
		if (debounce) {
			delay(11);
		}
		if (raw) {
			TEST_ASSERT_FALSE_MESSAGE(gpio_handler.getWatchedPins()[0].raw_state,
					"The GPIOHandler detected the high pin state despite interrupts being disabled.");
		} else {
			TEST_ASSERT_FALSE_MESSAGE(gpio_handler.getState(IN_PIN),
					"The GPIOHandler detected the high pin state despite interrupts being disabled.");
		}
		gpio_handler.checkPins();
	}
	if (debounce) {
		if (!raw) {
			TEST_ASSERT_FALSE_MESSAGE(gpio_handler.getState(IN_PIN),
					"The input pin state was detected as high before the debounce timeout was over.");
		}
		delay(11);
	}
	if (raw) {
		TEST_ASSERT_MESSAGE(gpio_handler.getWatchedPins()[0].raw_state,
				"The GPIOHandler failed to detect the pin state change to high.");
		TEST_ASSERT_NOT_EQUAL_MESSAGE(0,
				gpio_handler.getWatchedPins()[0].raw_last_change,
				"raw_last_change was still 0 after a state change.");
	} else {
		TEST_ASSERT_MESSAGE(gpio_handler.getState(IN_PIN),
				"The GPIOHandler failed to detect the pin state change to high.");
		TEST_ASSERT_EQUAL_MESSAGE(changes, gpio_handler.getChanges(IN_PIN),
				"The input pin state change counter was incorrect after the first change.");
		TEST_ASSERT_NOT_EQUAL_MESSAGE(0,
				gpio_handler.getWatchedPins()[0].last_change,
				"last_change was still 0 after a state change.");
	}

	// Make sure that changing the input resistor of a connected pin doesn't change its state.
	gpio_handler.updateGPIO(IN_PIN, "Test", false);
	if(!interrupt) {
		gpio_handler.checkPins();
	}
	if (debounce) {
		delay(11);
	}
	if (raw) {
		TEST_ASSERT_MESSAGE(gpio_handler.getWatchedPins()[0].raw_state,
				"Changing the input resistor of a connected pin changed its raw state.");
	} else {
		TEST_ASSERT_MESSAGE(gpio_handler.getState(IN_PIN),
				"Changing the input resistor of a connected pin changed its state.");
	}

	// Bounce output pin a bit and set it low.
	uint8_t bounces = rand() % 25 + 1;
	bounce_pin(OUT_PIN, 9, false, bounces);
	if (!debounce && interrupt) {
		changes += bounces * 2 - 1;
	} else {
		changes++;
	}
	if (!interrupt) {
		gpio_handler.checkPins();
	}
	if (debounce) {
		delay(11);
	}
	if (raw) {
		TEST_ASSERT_FALSE_MESSAGE(gpio_handler.getWatchedPins()[0].raw_state,
				"Input pin raw_state after bouncing was not detected as low by GPIOHandler.");
	} else {
		TEST_ASSERT_FALSE_MESSAGE(gpio_handler.getState(IN_PIN),
				"Input pin state after bouncing was not detected as low by GPIOHandler.");
		TEST_ASSERT_EQUAL_MESSAGE(changes, gpio_handler.getChanges(IN_PIN),
				"The input pin state change counter was incorrect after some bouncing.");
	}

	// Test much quicker bouncing.
	bounces = rand() % 25 + 1;
	bounce_pin(OUT_PIN, 3, true, bounces);
	if (!debounce && interrupt) {
		changes += bounces * 2 - 1;
	} else {
		changes++;
	}
	if (!interrupt) {
		gpio_handler.checkPins();
	}
	if (debounce) {
		delay(11);
	}
	if (raw) {
		TEST_ASSERT_MESSAGE(gpio_handler.getWatchedPins()[0].raw_state,
				"Input pin raw_state after bouncing was not detected as high by GPIOHandler.");
	} else {
		TEST_ASSERT_MESSAGE(gpio_handler.getState(IN_PIN),
				"Input pin state after bouncing was not detected as high by GPIOHandler.");
		TEST_ASSERT_EQUAL_MESSAGE(changes, gpio_handler.getChanges(IN_PIN),
				"The input pin state change counter was incorrect after some bouncing.");
	}

	// Test IN_PIN_2 being low with a pull down resistor.
	gpio_handler.registerGPIO(IN_PIN_2, "Test Pin", false);
	size_t in_pin_2_index = 0;
	for (pin_state pin : gpio_handler.getWatchedPins()) {
		if (pin.number == IN_PIN_2) {
			break;
		}
		in_pin_2_index++;
	}
	if (raw) {
		TEST_ASSERT_FALSE_MESSAGE(gpio_handler.getWatchedPins()[in_pin_2_index].raw_state,
				"Input pin 2 raw_state was not detected as low after registering.");
		TEST_ASSERT_EQUAL_MESSAGE(0,
				gpio_handler.getWatchedPins()[in_pin_2_index].raw_last_change,
				"raw_last_change was not 0 after registering input pin 2.");
	} else {
		TEST_ASSERT_FALSE_MESSAGE(gpio_handler.getState(IN_PIN_2),
				"Input pin 2 state was not detected as low after registering.");
		TEST_ASSERT_EQUAL_MESSAGE(0, gpio_handler.getChanges(IN_PIN_2),
				"The input 2 pin state change counter was not 0 after registering.");
		TEST_ASSERT_EQUAL_MESSAGE(0, gpio_handler.getWatchedPins()[in_pin_2_index].last_change,
				"last_change was not 0 after registering input pin 2.");
	}

	// Make sure IN_PIN_2 state changes to high when changing input resistor.
	gpio_handler.updateGPIO(IN_PIN_2, "Test Pin", true);
	if (debounce) {
		if (!raw) {
			TEST_ASSERT_FALSE_MESSAGE(gpio_handler.getState(IN_PIN_2),
					"Input pin 2 state was detected as high before the debouncing timeout was over.");
		}
		delay(11);
	}
	if (raw) {
		TEST_ASSERT_MESSAGE(gpio_handler.getWatchedPins()[in_pin_2_index].raw_state,
				"Input pin 2 raw_state was not detected as high after changing its input resistor.");
		TEST_ASSERT_NOT_EQUAL_MESSAGE(0,
				gpio_handler.getWatchedPins()[in_pin_2_index].raw_last_change,
				"raw_last_change was 0 after changing the input resistor for input pin 2.");
	} else {
		TEST_ASSERT_MESSAGE(gpio_handler.getState(IN_PIN_2),
				"Input pin 2 state was not detected as high after changing its input resistor.");
		TEST_ASSERT_NOT_EQUAL_MESSAGE(0, gpio_handler.getChanges(IN_PIN_2),
				"The input 2 pin state change counter was 0 after changing its input resistor.");
		TEST_ASSERT_NOT_EQUAL_MESSAGE(0, gpio_handler.getWatchedPins()[in_pin_2_index].last_change,
				"last_change was 0 after changing the input resistor for input pin 2.");
	}

	// Reset gpio handler.
	gpio_handler.unregisterGPIO(IN_PIN);
	gpio_handler.unregisterGPIO(IN_PIN_2);
	gpio_handler.enableInterrupts();
	gpio_handler.setDebounceTimeout(10);
}
