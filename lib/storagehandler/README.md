# Storage Handler
The Storage Handler stores and loads pin states to/from persistent store, for example SPIFFS.  
It can either store a vector of pin state objects, or a [GPIO Handler](../gpiohandler/README.md).

The Storage Handler is given a filesystem and a path upon creation, in which it then stores the given pins whenever `storePins` or `storeGPIOHandler` is called.  
Both the path and the filesystem can be changed after creation.

Storing a [GPIO Handler](../gpiohandler/README.md) using `storeGPIOHandler` is preferred since it disables pin interrupts and pin debouncing while storing the pins.  
This is important, because a pin interrupt being called while the Flash writes causes a crash.

Loading a [GPIO Handler](../gpiohandler/README.md) using `loadGPIOHandler` completely overrides the current content of the [GPIO Handler](../gpiohandler/README.md).  
This means it will add any pins that only exist in the file, updates ones existing in both, and removes those not existing in the file.  
Loading a [GPIO Handler](../gpiohandler/README.md) also disables pin interrupts and pin debouncing while reading.

While there is a default instance there should be no problems what so ever with creating additional instances.  
Both on the same filesystem, as well as on different ones.  
In theory there should also be no problems with two Storage Handlers using the same file, however this will cause issues if they try to read/write at the same time.
