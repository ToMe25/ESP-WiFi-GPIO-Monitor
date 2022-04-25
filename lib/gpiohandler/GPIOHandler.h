/*
 * gpiohandler.h
 *
 *  Created on: 05.03.2022
 *      Author: ToMe25
 */

#ifndef SRC_GPIOHANDLER_H_
#define LIB_GPIOHANDLER_H_

#include <Arduino.h>
#include <map>
#include <unordered_set>
#include <vector>

struct pin_state;

/**
 * The different errors that can occur when registering, unregistering or updating a watched pin.
 */
enum gpio_err_t {
	GPIO_OK,
	GPIO_PIN_INVALID,
	GPIO_NAME_INVALID,
	GPIO_ALREADY_WATCHED,
	GPIO_NOT_WATCHED
};

class GPIOHandler {
public:
	/**
	 * The default GPIOHandler constructor creating a new GPIOHandler.
	 */
	GPIOHandler();

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
	 * Gets the name of the given pin to be shown to the user.
	 * Returns an empty string if the pin isn't watched.
	 *
	 * @param pin	The pin to get the name for.
	 * @return	The name of the given pin.
	 */
	String getName(const uint8_t pin) const;

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
private:
	/**
	 * The handle used to interact with the debounce timer.
	 */
	esp_timer_handle_t debounce_timer = NULL;

	/**
	 * The pins that are currently being watched.
	 */
	std::map<uint8_t, pin_state> watched;

	/**
	 * Whether interrupts should be used.
	 */
	bool interrupts = true;

	/**
	 * A vector containing pointers to all the pin states that have changed and are to be watched for debouncing.
	 */
	std::vector<pin_state*> dirty_states;

	/**
	 * The config used for the debounce timer.
	 */
	esp_timer_create_args_t debounce_timer_config = {
		timerInterrupt,
		this,
		ESP_TIMER_TASK,
		"Debouncer",
		false,
	};

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
	 */
	pin_state(GPIOHandler *handler, const uint8_t pin_nr,
			const String name, const bool pull_up, const bool state) :
			handler(handler), number(pin_nr), name(name), pull_up(pull_up), state(state), raw_state(state) {
	}

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
	volatile uint64_t changes = 0;

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

#endif /* LIB_GPIOHANDLER_H_ */
