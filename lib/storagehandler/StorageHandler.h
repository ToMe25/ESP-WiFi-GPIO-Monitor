/*
 * StorageHandler.h
 *
 *  Created on: 07.05.2022
 *      Author: ToMe25
 */

#ifndef LIB_STORAGEHANDLER_STORAGEHANDLER_H_
#define LIB_STORAGEHANDLER_STORAGEHANDLER_H_

#include "GPIOHandler.h"
#include <SPIFFS.h>

/**
 * The different return states(errors and OK) that can occur when interacting with the flash storage.
 */
enum storage_err_t {
	STORAGE_OK,
	STORAGE_PATH_NULL,
	STORAGE_IS_DIR,
	STORAGE_NOT_FOUND,
	STORAGE_OPEN_FAIL,
	STORAGE_WRITE_ERR
};

class StorageHandler {
public:
	/**
	 * Creates a new StorageHandler storing pin data at the given location.
	 *
	 * @param file_system		The filesystem on which to put the file storing the pin data.
	 * @param pin_storage_path	The path to the file in which the pin data should be stored.
	 */
	StorageHandler(fs::FS &file_system = SPIFFS, const char *pin_storage_path =
			"/pins.csv") :
			fs(file_system), pin_storage(pin_storage_path) {
	}

	/**
	 * Destroys this StorageHandler.
	 */
	virtual ~StorageHandler() {
	}

	/**
	 * Stores the state of all the pins watched by the given GPIOHandler to this StorageHandlers pin storage file.
	 * Most likely just calls storePins with the pins watched by the GPIOHandler.
	 * Overrides the file, meaning you can't store two GPIOHandlers in the same file at the same time.
	 * Disables pin interrupts while writing the file, to prevent crashes.
	 *
	 * @param handler	The GPIOHandler to store.
	 * @return	What went wrong when trying to store the given GPIOHandler in the flash.
	 * 			STORAGE_OK if nothing went wrong.
	 */
	storage_err_t storeGPIOHandler(GPIOHandler &handler);

	/**
	 * Stores all the pin_state objects from the given vector to the pin storage file.
	 * Overrides the previous content of the file.
	 * If storing a complete GPIOHandler it is recommended to use storeGPIOHandler.
	 *
	 * @param pins	A vector containing the pins to store.
	 * @return	What went wrong when trying to store the given GPIOHandler in the flash.
	 * 			STORAGE_OK if nothing went wrong.
	 */
	storage_err_t storePins(const std::vector<pin_state> &pins);

	/**
	 * Reads the pin storage file and registers all pins found in it.
	 * Overriding them if they are already registered.
	 * Removes pins that are registered but not found in the pin storage file.
	 *
	 * @param handler	The GPIOHandler to load the pins into.
	 * @return	What went wrong when trying to load the given GPIOHandler from the flash.
	 * 			STORAGE_OK if nothing went wrong.
	 */
	storage_err_t loadGPIOHandler(GPIOHandler &handler) const;

	/**
	 * Sets the path of the file in which to store the pin states in the future.
	 * Does not delete the previous file, nor does it copy its content.
	 * Setting this to NULL basically disables pin storage.
	 *
	 * @param pin_storage_path	The path at which to store pin_state objects.
	 */
	void setPinStoragePath(const char *pin_storage_path);

	/**
	 * Gets the current path of the file to store the pin states in.
	 *
	 * @return	The path storePins and storeGPIOHandler store pins to.
	 */
	const char* getPinStoragePath() const;

	/**
	 * Sets the file system on which to store and from which to load pin states.
	 *
	 * @param file_system	The new file system to use for data storage.
	 */
	void setFileSystem(fs::FS &file_system);

	/**
	 * Gets the file system currently used to store the pin storage file.
	 *
	 * @return	The current file system.
	 */
	fs::FS& getFileSystem() const;

	/**
	 * Gets the write error the file system returned in the last write.
	 * 0 if everything is ok.
	 *
	 * @return	The write error from the last storePins call.
	 */
	int getWriteError() const;

private:
	/**
	 * The file system to store the GPIO pin states onto.
	 */
	fs::FS &fs;

	/**
	 * The path of the file on the file system to store the pin states in.
	 */
	const char *pin_storage;

	/**
	 * The write error from the last time writing pin states to the flash.
	 */
	int write_error = 0;
};

extern StorageHandler storage_handler;

#endif /* LIB_STORAGEHANDLER_STORAGEHANDLER_H_ */
