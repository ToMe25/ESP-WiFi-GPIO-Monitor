# TODO
A list of changes/additions for this program to implement in the future.
 * Web interface password protection
 * Prometheus Web interface usage statistics(number of requests, and maybe response times)
 * Per pin debounce timeout
 * Analog read support
 * ESP8266 support
 * Move unit tests to [ArduinoFake](https://github.com/FabioBatSilva/ArduinoFake)?
 * Make web interface add/remove watched pins without reloading page using javascript
 * Make web interface javascript use long polling to update web interfaces
 * Make config interface have delete confirmation without page reload using javascript
 * Add debounce timeout to settings page
 * Add a settings log showing the last X(20?) settings changes(general settings, pins added, pins updated, and pins deleted)
 * Make / automatically redirect to /settings.html if no pin is registered and /index.html otherwise
 * Change debounce timer to os_timer or Ticker
 * Change filesystem to LittleFS with arduino-esp32 2.0.X
 * Make StorageHandler only rewrite the file if any of the pin states changed(either compared to the current file, or a cached list)
 * Store last pin state change time?
