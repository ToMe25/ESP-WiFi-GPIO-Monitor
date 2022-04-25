/*
 * gpiohandler_test.h
 *
 *  Created on: 11.03.2022
 *      Author: ToMe25
 */

#ifndef TEST_GPIOHANDLER_TEST_H_
#define TEST_GPIOHANDLER_TEST_H_

#include <Arduino.h>

/**
 * Simulate random pin bouncing on the given output pin.
 * Toggles the given output pin a random number of times between high and low.
 *
 * @param pin				The GPIO pin to toggle.
 * @param max_bounce_time	The max time the pin is allowed to stay in one state before the final state.
 * @param target			The target state of the time after all the bouncing. True means HIGH.
 * @param bounces			The number of times to toggle the state of the output pin.
 */
void bounce_pin(const uint8_t pin, const uint8_t max_bounce_time,
		const bool target, const uint8_t bounces);

/**
 * The method running all the GPIOHandler tests.
 */
void run_gpiohandler_tests();

/**
 * Tests registering, updating, and unregistering pins.
 * Also tests enabling/disabling interrupts.
 * As well as setting the debouncing timeout.
 */
void test_gpiohandler_methods();

/**
 * Tests whether the input and output pin are connected.
 */
void test_pins_connected();

/**
 * Tests whether the GPIOHandler correctly detects the raw state of the registered input pins.
 * Expects pins test_main.IN_PIN and test_main.OUT_PIN to be connected.
 * With pin interrupts enabled.
 */
void test_pin_raw_state_with_interrupt();

/**
 * Tests whether the GPIOHandler correctly detects the raw state of the registered input pins.
 * Expects pins test_main.IN_PIN and test_main.OUT_PIN to be connected.
 * With pin interrupts disabled.
 */
void test_pin_raw_state_without_interrupt();

/**
 * Tests whether the GPIOHandler correctly detects the current state of the registered input pins.
 * Expects pins test_main.IN_PIN and test_main.OUT_PIN to be connected.
 * With pin interrupts enabled.
 * With pin debouncing enabled.
 */
void test_pin_state_with_interrupt();

/**
 * Tests whether the GPIOHandler correctly detects the current state of the registered input pins.
 * Expects pins test_main.IN_PIN and test_main.OUT_PIN to be connected.
 * With pin interrupts disabled.
 * With pin debouncing enabled.
 */
void test_pin_state_without_interrupt();

/**
 * Tests whether the GPIOHandler correctly detects the current state of the registered input pins.
 * Expects pins test_main.IN_PIN and test_main.OUT_PIN to be connected.
 * With pin interrupts enabled.
 * With pin debouncing disabled.
 */
void test_pin_state_with_interrupt_without_debounce();

/**
 * Tests whether the GPIOHandler correctly detects the current state of the registered input pins.
 * Expects pins test_main.IN_PIN and test_main.OUT_PIN to be connected.
 * With pin interrupts disabled.
 * With pin debouncing disabled.
 */
void test_pin_state_without_interrupt_without_debounce();

/**
 * Tests whether the GPIOHandler correctly detects the current state of the registered input pins.
 * Expects pins test_main.IN_PIN and test_main.OUT_PIN to be connected.
 *
 * @param raw		If true checks raw pin states rather then the final states after debouncing(if enabled).
 * @param interrupt	If true pin interrupts are enabled.
 * @param debounce	If true pin debouncing is enabled for the test. Irrelevant for raw tests.
 */
void test_pin_state(bool raw, bool interrupt, bool debounce);

#endif /* TEST_GPIOHANDLER_TEST_H_ */
