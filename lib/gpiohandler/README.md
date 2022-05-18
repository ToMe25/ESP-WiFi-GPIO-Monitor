# GPIO Handler
The GPIO Handler detects, stores, and handles pin states.

The GPIO Handler stores this info about all registered pins:
 * a pins registered name
 * its hardware pin number
 * its raw state
 * the last time the raw state changed
 * the debounced state
 * the last time the debounced state changed
 * and the number number of changes of the debounced state

There are two ways for the hardware state and all values dependent on it to be updated: 
 1. After a pin is registered the GPIO Handler will register a pin interrupt to automatically detect changes to the hardware pin state.  
    This can be disabled by calling `disableInterrupts`.
 2. When `checkPins` is called the GPIO Handler will check the current state of all pins.

Either way, unless debouncing is disabled(by setting the debounce timeout to 0), it will automatically debounce the pin state before it is stored.

Before a pin is registered the GPIO Handler makes sure it is a valid GPI or GPIO pin, that the given name is valid, and that the given pin is not connected to the internal flash.

While there is a global instance, creating a new one using different settings shouldn't be a problem.  
Please note however that only one instance can have a pin interrupt on the same pin at the same time.

## [Storage Handler](../storagehandler/README.md) integration
The GPIO Handler keeps a pointer to a [Storage Handler](../storagehandler/README.md) instance to automatically store its current info.  
If this pointer is `NULL` it will not attempt to store anything.  
When a pin is registered, updated(using `updateGPIO` or `setName`), or removed the GPIO Handler will automatically store all\* its current pin info to the [Storage Handler](../storagehandler/README.md).  
It will also store this info when the [Storage Handler](../storagehandler/README.md) is set, unless it is set to `NULL` or the `write` parameter is set to false.

\* The info stored does not include last change time atm.  
It also does not contain raw state and change time, as those are considered too volatile to reasonably store.

Calling `writeToStorageHandler` will write the current pin states if `force` is set, or at least one pin state changed since the last time the GPIO Handler was written to the [Storage Handler](../storagehandler/README.md).
