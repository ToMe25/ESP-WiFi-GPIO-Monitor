/*
 * gpiohandler.h
 *
 *  Created on: 05.03.2022
 *      Author: ToMe25
 */

#ifndef LIB_GPIOHANDLER_GPIOHANDLER_H_
#define LIB_GPIOHANDLER_GPIOHANDLER_H_

#include <Arduino.h>
#include "driver/timer.h"
#include <map>
#include <unordered_set>

/**
 * Forward declaration to prevent circular inclusion.
 */
class StorageHandler;

/**
 * Forward declaration because the struct needs a GPIOHandler instance.
 */
struct pin_state;

/**
 * A helper struct representing a ESP32 hardware timer.
 */
struct hw_timer_index_t {
	timer_group_t group;
	timer_idx_t timer;
};

/**
 * The different errors that can occur when registering, unregistering or updating a watched pin.
 */
enum gpio_err_t {
	GPIO_OK,
	GPIO_PIN_INVALID,
	GPIO_NAME_INVALID,
	GPIO_ALREADY_WATCHED,
	GPIO_NOT_WATCHED,
	GPIO_FLASH_PIN
};

class GPIOHandler {
public:
	/**
	 * The default GPIOHandler constructor creating a new GPIOHandler.
	 *
	 * @param handler	The storage handler to use to store the pins in flash.
	 * 					Set to NULL to disable pin flash storage.
	 */
	GPIOHandler(StorageHandler *handler = NULL);

	/**
	 * Destroys the GPIOHandler.
	 */
	virtual ~GPIOHandler();

	/**
	 * Checks whether the given GPIO pin can be watched, and starts watching it if so.
	 * This method first checks whether the pin with the given number is a GPIO pin.
	 * Then it checks whether the given ping is already watched.
	 * After this it sets the pin to input with a pull up or pull down resistor.
	 * Then it registers the pin to actually be watched.
	 *
	 * NOTE: While registering, updating(name and/or resistor), and unregistering
	 * a pin automatically writes the changes to the StorageHandler,
	 * a pin changing its state DOES NOT.
	 *
	 * @param pin		The pin which should be watched for updates.
	 * @param name		The name of the pin for external services.
	 * 					Has to be at least three characters in length.
	 * 					Has to be 32 or less characters long.
	 * 					Can contain letters, digits, spaces, underscored, and hyphens.
	 * @param pull_up	Whether to use an internal pull up resistor or a pull down one.
	 * @return	What went wrong when registering the pin to be watched.
	 * 			GPIO_OK if the pin was registered successfully.
	 */
	gpio_err_t registerGPIO(const uint8_t pin, String name, const bool pull_up);

	/**
	 * Stops the given pin from being watched and removes all records about it.
	 *
	 * NOTE: While registering, updating(name and/or resistor), and unregistering
	 * a pin automatically writes the changes to the StorageHandler,
	 * a pin changing its state DOES NOT.
	 *
	 * @param pin	The pin which should no longer be watched.
	 * @return	What went wrong when unregistering the pin to be watched.
	 * 			GPIO_OK if the pin was unregistered successfully.
	 */
	gpio_err_t unregisterGPIO(const uint8_t pin);

	/**
	 * Updates the name and/or resistor of an already watched pin.
	 * Can change the pin name and/or pull up/down resistor.
	 * Only works if the pin was already watched.
	 *
	 * NOTE: While registering, updating(name and/or resistor), and unregistering
	 * a pin automatically writes the changes to the StorageHandler,
	 * a pin changing its state DOES NOT.
	 *
	 * @param pin		The pin to update.
	 * @param name		The new name of the pin being updated.
	 * 					Has to be at least three characters in length.
	 * 					Has to be 32 or less characters long.
	 * 					Can contain letters, digits, spaces, underscored, and hyphens.
	 * @param pull_up	Whether the pin should use an internal pull up resistor rather then a pull down one.
	 * @return	What went wrong when updating the pin to be watched.
	 * 			GPIO_OK if the pin was updated successfully.
	 */
	gpio_err_t updateGPIO(const uint8_t pin, String name, const bool pull_up);

	/**
	 * Check the states of all watched pins and update their states.
	 */
	void checkPins();

	/**
	 * Gets the current state of the given pin.
	 * Returns false if the pin isn't watched, or if it is low.
	 *
	 * @param pin	The pin to check.
	 * @return	True if the pin is high and watched.
	 */
	bool getState(const uint8_t pin) const;

	/**
	 * Gets the current number of pin changes on the given pin.
	 * Returns UINT64_MAX if the pin isn't watched.
	 *
	 * @param pin	The pin to get the number of state changes for.
	 * @return	The number of state changes, or UINT64_MAX if it isn't watched.
	 */
	uint64_t getChanges(const uint8_t pin) const;

	/**
	 * Sets the current number of pin state changes for a given pin.
	 *
	 * @param pin		The pin to set the number of changes for.
	 * @param changes	The new number of changes to set for the given pin.
	 * @return	What went wrong when updating the pin.
	 * 			GPIO_OK if the pin was updated successfully.
	 */
	gpio_err_t setChanges(const uint8_t pin, const uint64_t changes);

	/**
	 * Gets the name of the given pin to be shown to the user.
	 * Returns an empty string if the pin isn't watched.
	 *
	 * @param pin	The pin to get the name for.
	 * @return	The name of the given pin.
	 */
	String getName(const uint8_t pin) const;

	/**
	 * Sets the name for a given pin.
	 *
	 * NOTE: While registering, updating(name and/or resistor), and unregistering
	 * a pin automatically writes the changes to the StorageHandler,
	 * a pin changing its state DOES NOT.
	 *
	 * @param pin	The pin to set the name for.
	 * @param name	The new name of the pin being updated.
	 * 				Has to be at least three characters in length.
	 * 				Has to be 32 or less characters long.
	 * 				Can contain letters, digits, spaces, underscored, and hyphens.
	 */
	gpio_err_t setName(const uint8_t pin, String name);

	/**
	 * Checks whether the given pin is being watched.
	 *
	 * @param pin	The pin to check.
	 * @return	True if the given pin is already being watched.
	 */
	bool isWatched(const uint8_t pin) const;

	/**
	 * Gets the pin state objects for all the watched pins.
	 *
	 * @return	A vector containing all the pin_state objects for currently watched pins.
	 */
	std::vector<pin_state> getWatchedPins() const;

	/**
	 * Removes all existing pin interrupts and disables creation of new ones.
	 * To disable pin debouncing use setDebounceTimeout to set the debounce timeout to zero.
	 */
	void disableInterrupts();

	/**
	 * Adds pin interrupts for all watched pins and enables creation of new ones.
	 */
	void enableInterrupts();

	/**
	 * Checks whether interrupts are currently enabled for this GPIOHandler.
	 *
	 * @return	Whether interrupts are currently enabled.
	 */
	bool interrupsEnabled() const;

	/**
	 * Sets the time a pin has to stay in the same state for its state to be considered changed.
	 * Set to zero to disable pin debouncing entirely.
	 *
	 * @param timeout	The debouncing timeout.
	 */
	void setDebounceTimeout(const uint16_t timeout);

	/**
	 * Gets the current time a pin has to stay in the same state for its state to be considered changed.
	 *
	 * @return	The debouncing timeout.
	 */
	uint16_t getDebounceTimeout() const;

	/**
	 * Sets the storage handler to use to store pin states in the flash.
	 * Set to NULL to disable storing pin states in the flash.
	 *
	 * @param handler	The new StorageHandler to use to store pin states in the flash.
	 * @param write		Whether this GPIOHandler should be written to the new StorageHandler.
	 * 					Only writes if this is true and the new handler isn't the old handler.
	 */
	void setStorageHandler(StorageHandler *handler, bool write = true);

	/**
	 * Gets the StorageHandler currently used to store pin states in the flash.
	 */
	StorageHandler* getStorageHandler() const;

	/**
	 * Writes the states of all the pins watched by this GPIOHandler to the flash.
	 * If any pin changed its state since the last time this GPIOHandler was written.
	 * Gets called automatically when changing the StorageHandler,
	 * registering a pin, unregistering a pin, or changing a pin name/resistor.
	 *
	 * @param force	Set to true to write the pin states even if they didn't change.
	 */
	void writeToStorageHandler(bool force = false);

	/**
	 * Checks whether the given pin is a valid input pin to watch.
	 * Makes sure the following criteria are met:
	 *  * The pin is a GPI or GPIO pin.
	 *  * The pin is not connected to the internal flash.
	 *
	 * @param pin	The pin to check.
	 * @return	GPIO_OK if the pin can be watched, or an error if it can't.
	 */
	static gpio_err_t isValidPin(const uint8_t pin);
private:
	/**
	 * The timer used to check the pin state for debouncing.
	 */
	static constexpr hw_timer_index_t DEBOUNCE_TIMER = { TIMER_GROUP_0, TIMER_1 };

	/**
	 * The config used for the debounce timer.
	 */
	const timer_config_t debounce_timer_config = {
		false,
		false,
		TIMER_INTR_LEVEL,
		TIMER_COUNT_UP,
		false,
		80,
	};

	/**
	 * The pins that are currently being watched.
	 */
	std::map<uint8_t, pin_state> watched;

	/**
	 * Whether interrupts should be used.
	 */
	bool interrupts = true;

	/**
	 * The storage handler used to write the pin states to flash.
	 */
	StorageHandler *storage;

	/**
	 * A vector containing pointers to all the pin states that have changed and are to be watched for debouncing.
	 */
	std::vector<pin_state*> debouncing_states;

	/**
	 * Whether this GPIOHandler changed since the last time it was written to the flash.
	 */
	bool dirty = false;

	/**
	 * How long a pin has to stay in the same state for its state to be considered changed.
	 */
	volatile uint16_t debounce_timeout = 10;

	/**
	 * The method handling a pin changing its state.
	 * Requires the pin_state for the watched to be given as the arg.
	 *
	 * @param arg	An arg given by the interrupt. Expected to be the pin_state for the changed pin.
	 */
	static void IRAM_ATTR pinInterrupt(void *arg);

	/**
	 * The method handling a timer interrupt and updating the pin state objects.
	 * This method does the debouncing.
	 * Requires an instance of GPIOHandler to be given as the argument.
	 *
	 * @param arg	An arg given by the timer. Expected to be the GPIOHandler to update.
	 */
	static void IRAM_ATTR timerInterrupt(void *arg);

	/**
	 * Updates the given pin state based on the current hardware status.
	 *
	 * @param pin	A pointer to the pin_state object to update.
	 */
	void IRAM_ATTR updatePin(pin_state *pin);

	/**
	 * Starts the timer again so the callback gets called after the given time.
	 *
	 * @param delay	The time in milliseconds after which the timer callback should be called.
	 */
	void IRAM_ATTR startTimer(const uint16_t delay);

	/**
	 * Checks whether the given char is valid for a pin name.
	 * Valid chars are the letters a to z, both upper and lower case.
	 * As well as digits, spaces, underscores, and hyphens.
	 *
	 * @param c	The char to check.
	 * @return	Whether the character is valid.
	 */
	static bool isValidNameChar(const char c);

	/**
	 * Checks whether the given string is a valid pin name.
	 * Pin names have to be 3 to 32 characters long.
	 * Pin names can contain letters(A-Z and a-z), digits(0-9), spaces, underscores, and hyphens.
	 *
	 * @param name	The name to check.
	 * @return	Whether the given name is valid for a GPIO pin.
	 */
	static bool isValidName(const String &name);
};

struct pin_state {
	/**
	 * Creates a new pin_state object.
	 *
	 * @param handler	A pointer to the GPIOHandler instance that manages this pin_state object.
	 * @param pin_nr	The hardware pin which this pin_state represents.
	 * @param name		The user facing name to use for this pin.
	 * @param pull_up	Whether this pin uses an internal pull up resistor instead of a pull down one.
	 * @param state		The initial state of this pin. Used for state and raw_state.
	 * @param changes	The number of pin state changes this pin has already registered.
	 */
	pin_state(GPIOHandler *handler, const uint8_t pin_nr,
			const String name,
			const bool pull_up, const bool state, const uint64_t changes = 0) :
			handler(handler), number(pin_nr), name(name), pull_up(pull_up), state(
					state), changes(changes), raw_state(state) {
	}

	/**
	 * Creates a new pin_state object copying the properties from the given pin state object.
	 *
	 * @param pin	The pin_state to copy.
	 */
	pin_state(const pin_state &pin) :
			handler(pin.handler), number(pin.number), name(pin.name), pull_up(
					pin.pull_up), state(pin.state), last_change(
					pin.last_change), changes(pin.changes), raw_state(
					pin.raw_state), raw_last_change(pin.raw_last_change) {
	}

	virtual ~pin_state() {
	}

	/**
	 * The instance of GPIOHandler this pin state is connected to.
	 */
	GPIOHandler *handler;

	/**
	 * Which GPI/GPIO pin is represented by this object.
	 */
	uint8_t number;

	/**
	 * The name of this pin for external software and the user.
	 */
	String name;

	/**
	 * Whether this pin uses an internal pull up resistor instead of a pull down one.
	 */
	bool pull_up;

	/**
	 * Whether the pin is currently high or low.
	 */
	volatile bool state;

	/**
	 * The last time the state of this pin changed.
	 */
	volatile uint64_t last_change = 0;

	/**
	 * The number of times this pin changed its state.
	 */
	volatile uint64_t changes;

	/**
	 * The current state of the hardware pin.
	 * Not debounced yet, to be used mainly for debouncing.
	 */
	volatile bool raw_state;

	/**
	 * The last time the hardware pin changed its state.
	 * Not debounced yet, to be used mainly for debouncing.
	 */
	volatile uint64_t raw_last_change = 0;
};

extern GPIOHandler gpio_handler;

#endif /* LIB_GPIOHANDLER_GPIOHANDLER_H_ */
