/*
 * StorageHandler.cpp
 *
 *  Created on: 07.05.2022
 *      Author: ToMe25
 */

#include "StorageHandler.h"

StorageHandler storage_handler;

StorageHandler::StorageHandler(fs::FS &file_system,
		const char *pin_storage_path) :
		fs(&file_system), pin_storage(pin_storage_path) {
}

storage_err_t StorageHandler::storeGPIOHandler(GPIOHandler &handler) {
	StorageHandler *storage = handler.getStorageHandler();
	handler.setStorageHandler(NULL);
	bool interrupts = handler.interrupsEnabled();
	handler.disableInterrupts();
	uint16_t debounce = handler.getDebounceTimeout();
	handler.setDebounceTimeout(0);

	storage_err_t err = storePins(handler.getWatchedPins());

	handler.setStorageHandler(storage, false);
	if (interrupts) {
		handler.enableInterrupts();
		handler.checkPins();
	}
	handler.setDebounceTimeout(debounce);
	return err;
}

storage_err_t StorageHandler::storePins(const std::vector<pin_state> &pins) {
	if (pin_storage == NULL) {
		return STORAGE_PATH_NULL;
	}

	if (fs->exists(pin_storage)) {
		fs::File file = fs->open(pin_storage);
		if (file.isDirectory()) {
			return STORAGE_IS_DIR;
		}
	}

	fs::File storage_file = fs->open(pin_storage, FILE_WRITE);

	if (!storage_file) {
		return STORAGE_OPEN_FAIL;
	}

	storage_file.println("Pin,Name,Resistor,State,Changes");

	for (pin_state state : pins) {
		storage_file.printf("%hu,%s,%hu,%hu,%llu\n", state.number, state.name.c_str(),
				state.pull_up, state.state, state.changes);
	}

	storage_file.close();
	write_error = storage_file.getWriteError();

	if (write_error == 0) {
		return STORAGE_OK;
	} else {
#if CORE_DEBUG_LEVEL >= 3
		Serial.print("Filesystem write error: ");
		Serial.println(write_error);
#endif
		return STORAGE_WRITE_ERR;
	}
}

storage_err_t StorageHandler::loadGPIOHandler(GPIOHandler &handler) const {
	if (pin_storage == NULL) {
		return STORAGE_PATH_NULL;
	}

	if (!fs->exists(pin_storage)) {
		return STORAGE_NOT_FOUND;
	}

	fs::File storage_file = fs->open(pin_storage);

	if (!storage_file) {
		return STORAGE_OPEN_FAIL;
	}

	if (storage_file.isDirectory()) {
		return STORAGE_IS_DIR;
	}

	StorageHandler *storage = handler.getStorageHandler();
	handler.setStorageHandler(NULL);
	bool interrupts = handler.interrupsEnabled();
	handler.disableInterrupts();
	uint16_t debounce = handler.getDebounceTimeout();
	handler.setDebounceTimeout(0);

	std::unordered_set<uint8_t> stored_pins;

	for (String line; storage_file.available() > 0;) {
		line = storage_file.readStringUntil('\n');
		if (line.length() == 0 || !isDigit(line[0])) {
			continue;
		}

		uint8_t number = 0;
		String name = "";
		bool pull_up = false;
		bool state = false;
		uint64_t changes = 0;
		for (int pos = 0, i = 0; pos >= 0; i++) {
			int end = line.indexOf(',', pos);
			String value = line.substring(pos, end);
			switch(i) {
			case 0:
				number = atoi(value.c_str());
				break;
			case 1:
				name = value;
				break;
			case 2:
				pull_up = value[0] == '1';
				break;
			case 3:
				state = value[0] == '1';
				break;
			case 4:
				changes = atoll(value.c_str());
				break;
			}
			pos = end > 0 ? end + 1 : end;
		}

		if (handler.isWatched(number)) {
			handler.updateGPIO(number, name, pull_up);
		} else {
			handler.registerGPIO(number, name, pull_up);
		}
		handler.setChanges(number,
				state == handler.getState(number) ? changes : changes + 1);
		stored_pins.insert(number);
	}

	storage_file.close();

	for (pin_state &pin : handler.getWatchedPins()) {
		if (stored_pins.count(pin.number) == 0) {
			handler.unregisterGPIO(pin.number);
		}
	}

	handler.setStorageHandler(storage, false);
	if (interrupts) {
		handler.enableInterrupts();
		handler.checkPins();
	}
	handler.setDebounceTimeout(debounce);
	return STORAGE_OK;
}

void StorageHandler::setPinStoragePath(const char *pin_storage_path) {
	if (pin_storage_path == NULL || strlen(pin_storage_path) == 0) {
		pin_storage = NULL;
	} else {
		pin_storage = pin_storage_path;
	}
}

const char* StorageHandler::getPinStoragePath() const {
	return pin_storage;
}

void StorageHandler::setFileSystem(fs::FS &file_system) {
	fs = &file_system;
}

fs::FS& StorageHandler::getFileSystem() const {
	return *fs;
}

int StorageHandler::getWriteError() const {
	return write_error;
}
