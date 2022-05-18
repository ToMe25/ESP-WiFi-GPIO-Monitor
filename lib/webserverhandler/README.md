# Web Server Handler
The Web Server Handler is responsible for all interactions with browsers.  
It handles all calls to the internal web server, and contains all its callbacks.

The Web Server Handler provides a web interface to watch the current state of the registered pins.  
As well as the settings interface,  to register, update, and remove pins from the [GPIO Handler](../gpiohandler/README.md).  
Deleting a pin requires one to confirm deletion on an additional page.  
The settings interface currently does not support authentication.

The Web Server Handler is given the web server port upon creation, and is initialized by calling the `setup` function.

The Web Server Handler also publishes the pin states in a [prometheus](https://prometheus.io/) compatible format on `/metrics`.

In addition the Web Server Handler registers a `http` service to the mDNS provider.

Currently the Web Server Handler can only use the default [GPIO Handler](../gpiohandler/README.md) instance.

There is no default instance of the Web Server Handler, and creating multiple of them should cause no issues, as long as they are on different ports.
