/*
 * main.h
 *
 *  Created on: 10.03.2022
 *      Author: ToMe25
 */

#ifndef TEST_TEST_MAIN_H_
#define TEST_TEST_MAIN_H_

#include <Arduino.h>

/**
 * The output pin that to set to test changing input on IN_PIN.
 * Expected to be connected to IN_PIN.
 */
const uint8_t OUT_PIN = 14;

/**
 * The input pin to use for testing.
 * Expected to be connected to OUT_PIN.
 */
const uint8_t IN_PIN = 34;

/**
 * A secondary input pin to be used for testing.
 * Assumed to be connected to nothing.
 * Assumed to change its state when its pull up/down resistor is changed.
 */
const uint8_t IN_PIN_2 = 15;

/**
 * The setup method initializing everything and running the tests.
 */
void setup();

/**
 * The loop method for the arduino framework.
 */
void loop();

#endif /* TEST_TEST_MAIN_H_ */
