/*
 * storage_handler_test.h
 *
 *  Created on: 12.05.2022
 *      Author: ToMe25
 */

#ifndef TEST_STORAGE_HANDLER_TEST_H_
#define TEST_STORAGE_HANDLER_TEST_H_

#include "StorageHandler.h"

/**
 * The path of the file to use for testing pin storage.
 */
const char storage_path[] = "/test/pins.csv";

/**
 * The storage handler used for these unit tests.
 */
extern StorageHandler storage;

/**
 * Checks whether the given l
 */
void check_pin_line(const char *line, const pin_state *pin);

/**
 * Checks that the given pins is correctly registered with the GPIOHandler.
 *
 * @param handler	The GPIOHandler to check.
 * @param pin		The number of the pin to check for.
 * @param name		The expected name of the pin.
 * @param pull_up	The expected resistor of the pin.
 * @param state		The expected pin state of the pin.
 * @param changes	The expected number of state changes of the pin.
 */
void check_pin_state(const GPIOHandler &handler, const uint8_t pin,
		const char *name, const bool pull_up, const bool state,
		const uint64_t changes);

/**
 * Reads the file from the path on the given filesystem and returns its lines in a vector.
 *
 * @param fs	The filesystem to read the file from.
 * @param path	The path of the file to read.
 * @return	The lines the file contained. Does not contain empty lines.
 */
std::unique_ptr<std::vector<String>> read_file(fs::FS fs, const char *path);

/**
 * The method testing whether storing pins, or a GPIOHandler generates a correct file.
 */
void test_store();

/**
 * Runs all the tests about loading a file to a GPIOHandler.
 */
void test_load();

/**
 * Tests whether the GPIOHandler correctly handles storing itself to the flash.
 */
void test_gpiohandler();

#endif /* TEST_STORAGE_HANDLER_TEST_H_ */
