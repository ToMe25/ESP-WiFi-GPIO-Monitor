/*
 * main.cpp
 *
 *  Created on: 10.03.2022
 *      Author: ToMe25
 */

#include "test_main.h"
#include <unity.h>

void setup() {
	delay(2000);

	UNITY_BEGIN();

	run_gpiohandler_tests();
	run_webserver_tests();
	run_storagehandler_tests();

	UNITY_END();
}

void loop() {
	delay(100);
}
