/*
 * storage_handler.cpp
 *
 *  Created on: 12.05.2022
 *      Author: ToMe25
 */

#include "storage_handler_test.h"
#include "test_main.h"
#include <unity.h>
#include <SPIFFS.h>

StorageHandler storage(SPIFFS, storage_path);

std::unique_ptr<std::vector<String>> read_file(fs::FS fs, const char *path) {
	fs::File file = fs.open(path);
	std::unique_ptr<std::vector<String>> lines = std::unique_ptr<
			std::vector<String>>(new std::vector<String>());

	if (!file || file.isDirectory()) {
		return lines;
	}

	while (file.available()) {
		String line = file.readStringUntil('\n');
		line.trim();
		if (line.length() > 0) {
			lines->push_back(line);
		}
	}

	return lines;
}

void check_pin_line(const char *line, const pin_state *pin) {
	const int length = snprintf(NULL, 0, "%hu,%s,%hu,%hu,%llu", pin->number, pin->name.c_str(),
				pin->pull_up, pin->state, pin->changes) + 1;

	char *expected = new char[length];
	snprintf(expected, length, "%hu,%s,%hu,%hu,%llu\n", pin->number,
			pin->name.c_str(), pin->pull_up, pin->state, pin->changes);

	TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, line,
			"The expected pin line did not match the found pin line.");
}

void check_pin_state(const GPIOHandler &handler, const uint8_t pin,
		const char *name, const bool pull_up, const bool state,
		const uint64_t changes) {
	TEST_ASSERT_MESSAGE(handler.isWatched(pin), "The given pin is not being watched.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE(name, handler.getName(pin).c_str(),
			"The name of the watched pin was loaded incorrectly.");

	for (pin_state &state : handler.getWatchedPins()) {
		if (state.number == pin) {
			TEST_ASSERT_EQUAL_MESSAGE(pull_up, state.pull_up,
					"The resistor for the watched pin was incorrect.");
		}
	}

	TEST_ASSERT_EQUAL_MESSAGE(state, handler.getState(pin),
			"The state of the watched pin did not match what it should have been.");
	TEST_ASSERT_EQUAL_MESSAGE(changes, handler.getChanges(pin),
			"The number of changes was incorrect after loading it.");
}

void run_storagehandler_tests() {
	// Initialize SPIFFS since it is required for these tests.
	SPIFFS.begin(true);

	RUN_TEST(test_store);
	RUN_TEST(test_load);
	RUN_TEST(test_gpiohandler);
}

void test_store() {
	// Delete the storage file if it exists.
	if (SPIFFS.exists(storage_path)) {
		SPIFFS.remove(storage_path);
	}

	// Test storing an empty list of pins to a NULL storage path.
	storage.setPinStoragePath(NULL);
	std::vector<pin_state> pins;
	TEST_ASSERT_EQUAL_MESSAGE(STORAGE_PATH_NULL, storage.storePins(pins),
			"Storing an empty pin list to a NULL storage path returned an incorrect status code.");
	TEST_ASSERT_FALSE_MESSAGE(SPIFFS.exists(storage_path), "The storage file existed after storing to a NULL path.");

	// Test storing a pin to a NULL storage path.
	pins.push_back(pin_state(NULL, IN_PIN, "Test Pin", true, false, 15));
	TEST_ASSERT_EQUAL_MESSAGE(STORAGE_PATH_NULL, storage.storePins(pins),
			"Storing an empty pin list to a NULL storage path returned an incorrect status code.");
	TEST_ASSERT_FALSE_MESSAGE(SPIFFS.exists(storage_path), "The storage file existed after storing to a NULL path.");

	// Test storing to the root directory.
	// Not possible with SPIFFS as far as I can tell.
	//storage.setPinStoragePath("/");
	//TEST_ASSERT_EQUAL_MESSAGE(STORAGE_IS_DIR, storage.storePins(pins),
	//		"Storing to the root directory didn't return the correct status.");

	// Test storing an empty list of pins.
	storage.setPinStoragePath(storage_path);
	pins.clear();
	TEST_ASSERT_EQUAL_MESSAGE(STORAGE_OK, storage.storePins(pins), "Storing an empty list of pins failed.");
	TEST_ASSERT_MESSAGE(SPIFFS.exists(storage_path),
			"The storage file for the StorageHandler does not exist after storing an empty list of pins.");
	std::unique_ptr<std::vector<String>> file = read_file(SPIFFS, storage_path);
	TEST_ASSERT_EQUAL_MESSAGE(1, file->size(),
			"The storage file was not one line long after storing an empty list.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("Pin,Name,Resistor,State,Changes",
			file->at(0).c_str(),
			"The first line of the storage file did not match the expected csv header.");

	// Test storing a single pin to the storage file.
	pins.push_back(pin_state(NULL, IN_PIN, "Test Pin", true, false, 15));
	TEST_ASSERT_EQUAL_MESSAGE(STORAGE_OK, storage.storePins(pins), "Storing a single pin to the flash failed.");
	file = read_file(SPIFFS, storage_path);
	TEST_ASSERT_EQUAL_MESSAGE(2, file->size(),
			"The number of lines of the storage file did not match what it should have been.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("Pin,Name,Resistor,State,Changes",
			file->at(0).c_str(),
			"The first line of the storage file did not match the expected csv header.");
	check_pin_line(file->at(1).c_str(), &pins[0]);

	// Test storing three pins to the storage file.
	pins.push_back(
			pin_state(NULL, IN_PIN_2, "Some Other Test Pin", false, false,
					1050));
	pins.push_back(pin_state(NULL, 12, "III", false, true, 0));
	TEST_ASSERT_EQUAL_MESSAGE(STORAGE_OK, storage.storePins(pins), "Storing three pins to the flash failed.");
	file = read_file(SPIFFS, storage_path);
	TEST_ASSERT_EQUAL_MESSAGE(4, file->size(),
			"The number of lines of the storage file did not match what it should have been.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("Pin,Name,Resistor,State,Changes",
			file->at(0).c_str(),
			"The first line of the storage file did not match the expected csv header.");
	check_pin_line(file->at(1).c_str(), &pins[0]);
	check_pin_line(file->at(2).c_str(), &pins[1]);
	check_pin_line(file->at(3).c_str(), &pins[2]);

	// Clear GPIOHandler in case it still contains pins from failed tests.
	for (pin_state &pin : gpio_handler.getWatchedPins()) {
		gpio_handler.unregisterGPIO(pin.number);
	}

	// Test storing an empty GPIOHandler to the flash.
	TEST_ASSERT_EQUAL_MESSAGE(STORAGE_OK, storage.storeGPIOHandler(gpio_handler), "Storing an empty GPIOHandler failed.");
	file = read_file(SPIFFS, storage_path);
	TEST_ASSERT_EQUAL_MESSAGE(1, file->size(),
			"The storage file was not one line long after storing an empty list.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("Pin,Name,Resistor,State,Changes",
			file->at(0).c_str(),
			"The first line of the storage file did not match the expected csv header.");

	// Test storing a GPIOHandler with one pin.
	gpio_handler.registerGPIO(IN_PIN, "Test Pin", false);
	TEST_ASSERT_EQUAL_MESSAGE(STORAGE_OK, storage.storeGPIOHandler(gpio_handler), "Storing an empty GPIOHandler failed.");
	file = read_file(SPIFFS, storage_path);
	TEST_ASSERT_EQUAL_MESSAGE(2, file->size(),
			"The number of lines of the storage file did not match what it should have been.");
	check_pin_line(file->at(1).c_str(), &gpio_handler.getWatchedPins()[0]);

	// Test storing a GPIOHandler with three pins.
	gpio_handler.registerGPIO(IN_PIN_2, "Some Other Test Pin 2", true);
	gpio_handler.registerGPIO(12, "XII", false);
	TEST_ASSERT_EQUAL_MESSAGE(STORAGE_OK, storage.storeGPIOHandler(gpio_handler), "Storing an empty GPIOHandler failed.");
	file = read_file(SPIFFS, storage_path);
	TEST_ASSERT_EQUAL_MESSAGE(4, file->size(),
			"The number of lines of the storage file did not match what it should have been.");
	check_pin_line(file->at(1).c_str(), &gpio_handler.getWatchedPins()[0]);
	check_pin_line(file->at(2).c_str(), &gpio_handler.getWatchedPins()[1]);
	check_pin_line(file->at(3).c_str(), &gpio_handler.getWatchedPins()[2]);

	// Clear GPIOHandler for future tests.
	gpio_handler.unregisterGPIO(IN_PIN);
	gpio_handler.unregisterGPIO(IN_PIN_2);
	gpio_handler.unregisterGPIO(12);
}

void test_load() {
	// Delete the storage file if it exists.
	if (SPIFFS.exists(storage_path)) {
		SPIFFS.remove(storage_path);
	}

	// Test loading from a NULL path.
	storage.setPinStoragePath(NULL);
	TEST_ASSERT_EQUAL_MESSAGE(STORAGE_PATH_NULL,
			storage.loadGPIOHandler(gpio_handler),
			"Trying to load pins from a NULL path returned an invalid status.");

	// Test loading from the root directory.
	// Not possible with SPIFFS as far as I can tell.
	//storage.setPinStoragePath("/");
	//TEST_ASSERT_EQUAL_MESSAGE(STORAGE_IS_DIR,
	//		storage.loadGPIOHandler(gpio_handler),
	//		"Trying to load pins from the root directory returned an invalid status.");

	// Test loading pins from a nonexistent file.
	storage.setPinStoragePath(storage_path);
	TEST_ASSERT_EQUAL_MESSAGE(STORAGE_NOT_FOUND,
			storage.loadGPIOHandler(gpio_handler),
			"Trying to load pins from a file that doesn't exist returned an invalid status.");

	// Test removing a pin from a GPIOHandler by loading an empty file.
	fs::File storage_file = SPIFFS.open(storage_path, FILE_WRITE);
	storage_file.close();
	gpio_handler.registerGPIO(IN_PIN, "Test", false);
	TEST_ASSERT_EQUAL_MESSAGE(STORAGE_OK, storage.loadGPIOHandler(gpio_handler),
			"Trying to load an empty file returned an invalid status.");
	TEST_ASSERT_EQUAL_MESSAGE(0, gpio_handler.getWatchedPins().size(),
			"Loading an empty file did not delete the pin from the GPIOHandler.");

	// Test removing a pin from a GPIOHandler by loading a file only containing the header.
	storage_file = SPIFFS.open(storage_path, FILE_WRITE);
	storage_file.println("Pin,Name,Resistor,State,Changes");
	storage_file.close();
	gpio_handler.registerGPIO(IN_PIN, "Test Pin", true);
	TEST_ASSERT_EQUAL_MESSAGE(STORAGE_OK, storage.loadGPIOHandler(gpio_handler),
			"Trying to load an empty file returned an invalid status.");
	TEST_ASSERT_EQUAL_MESSAGE(0, gpio_handler.getWatchedPins().size(),
			"Loading a file only containing a header did not delete the pin from the GPIOHandler.");

	// Test loading a pin from SPIFFS.
	digitalWrite(OUT_PIN, LOW);
	storage_file = SPIFFS.open(storage_path, FILE_APPEND);
	storage_file.printf("%hu,%s,%hu,%hu,%llu\n", IN_PIN, "Test Pin", 0, 0, (uint64_t) 12);
	storage_file.close();
	TEST_ASSERT_EQUAL_MESSAGE(STORAGE_OK, storage.loadGPIOHandler(gpio_handler),
			"Trying to load a file returned an invalid status.");
	TEST_ASSERT_EQUAL_MESSAGE(1, gpio_handler.getWatchedPins().size(),
			"Loading a file containing one pin didn't cause the GPIOHandler to watch 1 pin.");
	check_pin_state(gpio_handler, IN_PIN, "Test Pin", false, false, 12);

	// Test loading a pin deleting other pins.
	gpio_handler.unregisterGPIO(IN_PIN);
	gpio_handler.registerGPIO(IN_PIN_2, "Some Other Test Pin", true);
	TEST_ASSERT_EQUAL_MESSAGE(STORAGE_OK, storage.loadGPIOHandler(gpio_handler),
			"Trying to load a file returned an invalid status.");
	TEST_ASSERT_EQUAL_MESSAGE(1, gpio_handler.getWatchedPins().size(),
			"Loading a file containing one pin didn't cause the GPIOHandler to watch 1 pin.");
	check_pin_state(gpio_handler, IN_PIN, "Test Pin", false, false, 12);

	// Test updating a pin from file.
	gpio_handler.updateGPIO(IN_PIN, "Some Other name", true);
	gpio_handler.setChanges(IN_PIN, 16215);
	TEST_ASSERT_EQUAL_MESSAGE(STORAGE_OK, storage.loadGPIOHandler(gpio_handler),
			"Trying to load a file returned an invalid status.");
	TEST_ASSERT_EQUAL_MESSAGE(1, gpio_handler.getWatchedPins().size(),
			"Loading a file containing one pin didn't cause the GPIOHandler to watch 1 pin.");
	check_pin_state(gpio_handler, IN_PIN, "Test Pin", false, false, 12);

	// Test loading three pins from a SPIFFS file.
	storage_file = SPIFFS.open(storage_path, FILE_APPEND);
	storage_file.printf("%hu,%s,%hu,%hu,%llu\n", IN_PIN_2, "Some Other Test Pin 2", 1, 1, (uint64_t) 32715893);
	storage_file.printf("%hu,%s,%hu,%hu,%llu\n", 12, "III", 0, 0, (uint64_t) 0);
	storage_file.close();
	TEST_ASSERT_EQUAL_MESSAGE(STORAGE_OK, storage.loadGPIOHandler(gpio_handler),
			"Trying to load a file returned an invalid status.");
	TEST_ASSERT_EQUAL_MESSAGE(3, gpio_handler.getWatchedPins().size(),
			"Loading a file containing three pins didn't cause the GPIOHandler to watch three pins.");
	check_pin_state(gpio_handler, IN_PIN, "Test Pin", false, false, 12);
	check_pin_state(gpio_handler, IN_PIN_2, "Some Other Test Pin 2", true, true, 32715893);
	check_pin_state(gpio_handler, 12, "III", false, false, 0);

	// Remove registered pins for future tests.
	gpio_handler.unregisterGPIO(IN_PIN);
	gpio_handler.unregisterGPIO(IN_PIN_2);
	gpio_handler.unregisterGPIO(12);
}

void test_gpiohandler() {
	// Delete the storage file if it exists.
	if (SPIFFS.exists(storage_path)) {
		SPIFFS.remove(storage_path);
	}

	// Test writing a pin to a NULL StorageHandler.
	gpio_handler.setStorageHandler(NULL);
	gpio_handler.registerGPIO(IN_PIN, "Test Pin", true);
	gpio_handler.writeToStorageHandler();
	TEST_ASSERT_FALSE_MESSAGE(SPIFFS.exists(storage_path),
			"Writing to a NULL StorageHandler created an output file.");

	// Test register writing to the flash.
	digitalWrite(OUT_PIN, LOW);
	gpio_handler.unregisterGPIO(IN_PIN);
	gpio_handler.setStorageHandler(&storage, false);
	TEST_ASSERT_FALSE_MESSAGE(SPIFFS.exists(storage_path),
			"Setting a StorageHandler with write false wrote to the flash.");
	gpio_handler.registerGPIO(IN_PIN, "Test", true);
	TEST_ASSERT_MESSAGE(SPIFFS.exists(storage_path),
			"The storage file didn't exist after a register call.");
	std::unique_ptr<std::vector<String>> file = read_file(SPIFFS, storage_path);
	TEST_ASSERT_EQUAL_MESSAGE(2, file->size(),
			"The storage file was not two lines long after registering a pin.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("Pin,Name,Resistor,State,Changes",
			file->at(0).c_str(),
			"The first line of the storage file did not match the expected csv header.");
	check_pin_line(file->at(1).c_str(), &gpio_handler.getWatchedPins()[0]);

	// Test update writing to the flash.
	gpio_handler.updateGPIO(IN_PIN, "Some Other Test Name", false);
	file = read_file(SPIFFS, storage_path);
	TEST_ASSERT_EQUAL_MESSAGE(2, file->size(),
			"The storage file was not two lines long after updating a pin.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("Pin,Name,Resistor,State,Changes",
			file->at(0).c_str(),
			"The first line of the storage file did not match the expected csv header.");
	check_pin_line(file->at(1).c_str(), &gpio_handler.getWatchedPins()[0]);

	// Test setName writing to the flash.
	gpio_handler.setName(IN_PIN, "III");
	file = read_file(SPIFFS, storage_path);
	TEST_ASSERT_EQUAL_MESSAGE(2, file->size(),
			"The storage file was not two lines long after updating a pin name.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("Pin,Name,Resistor,State,Changes",
			file->at(0).c_str(),
			"The first line of the storage file did not match the expected csv header.");
	check_pin_line(file->at(1).c_str(), &gpio_handler.getWatchedPins()[0]);

	// Make sure setChanges doesn't write to the flash.
	pin_state old_state = gpio_handler.getWatchedPins()[0];
	gpio_handler.setChanges(IN_PIN, 12);
	file = read_file(SPIFFS, storage_path);
	TEST_ASSERT_EQUAL_MESSAGE(2, file->size(),
			"The storage file was not two lines long after updating a pins changes.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("Pin,Name,Resistor,State,Changes",
			file->at(0).c_str(),
			"The first line of the storage file did not match the expected csv header.");
	check_pin_line(file->at(1).c_str(), &old_state);

	// Make sure that a not forced writeToStorageHandler writes after setChanges.
	gpio_handler.writeToStorageHandler();
	file = read_file(SPIFFS, storage_path);
	TEST_ASSERT_EQUAL_MESSAGE(2, file->size(),
			"The storage file was not two lines long after updating a pins changes.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("Pin,Name,Resistor,State,Changes",
			file->at(0).c_str(),
			"The first line of the storage file did not match the expected csv header.");
	check_pin_line(file->at(1).c_str(), &gpio_handler.getWatchedPins()[0]);

	// Make sure a pin state change doesn't write the current state.
	old_state = gpio_handler.getWatchedPins()[0];
	digitalWrite(OUT_PIN, HIGH);
	gpio_handler.checkPins();
	delay(11);
	file = read_file(SPIFFS, storage_path);
	TEST_ASSERT_EQUAL_MESSAGE(2, file->size(),
			"The storage file was not two lines long after updating a pin state.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("Pin,Name,Resistor,State,Changes",
			file->at(0).c_str(),
			"The first line of the storage file did not match the expected csv header.");
	check_pin_line(file->at(1).c_str(), &old_state);

	// Test not forced writeToStorageHandler after a state change
	gpio_handler.writeToStorageHandler();
	file = read_file(SPIFFS, storage_path);
	TEST_ASSERT_EQUAL_MESSAGE(2, file->size(),
			"The storage file was not two lines long after updating a pin state.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("Pin,Name,Resistor,State,Changes",
			file->at(0).c_str(),
			"The first line of the storage file did not match the expected csv header.");
	check_pin_line(file->at(1).c_str(), &gpio_handler.getWatchedPins()[0]);

	// Make sure not forced writeToStoragehandler doesn't write without a change.
	SPIFFS.remove(storage_path);
	gpio_handler.writeToStorageHandler();
	TEST_ASSERT_FALSE_MESSAGE(SPIFFS.exists(storage_path),
			"A not forced writeToStorageHandler call wrote a file without a GPIOHandler change.");

	// Test a forced writeToStorageHandler call.
	gpio_handler.writeToStorageHandler(true);
	TEST_ASSERT_MESSAGE(SPIFFS.exists(storage_path),
			"The storage file didn't exist after a forced writeToStorageHandler call.");
	file = read_file(SPIFFS, storage_path);
	TEST_ASSERT_EQUAL_MESSAGE(2, file->size(),
			"The storage file was not two lines long after a forced writeToStorageHandler call.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("Pin,Name,Resistor,State,Changes",
			file->at(0).c_str(),
			"The first line of the storage file did not match the expected csv header.");
	check_pin_line(file->at(1).c_str(), &gpio_handler.getWatchedPins()[0]);

	// Make sure a unregister call writes the change to the flash.
	gpio_handler.unregisterGPIO(IN_PIN);
	file = read_file(SPIFFS, storage_path);
	TEST_ASSERT_EQUAL_MESSAGE(1, file->size(),
			"The storage file was not one line long after unregistering a pin.");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("Pin,Name,Resistor,State,Changes",
			file->at(0).c_str(),
			"The first line of the storage file did not match the expected csv header.");
}
