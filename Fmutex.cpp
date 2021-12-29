/*
 * Fmutex.cpp
 *
 *  Created on: 1 Nov 2021
 *      Author: Gaylord
 */

#include "Fmutex.h"

Fmutex::Fmutex() {
	mutex = xSemaphoreCreateMutex();
}
Fmutex::~Fmutex() {
	/* delete semaphore */
	/* (not needed if object lifetime is known
	 * to be infinite) */
}
void Fmutex::lock() {
	xSemaphoreTake(mutex, portMAX_DELAY);
}
void Fmutex::unlock() {
	xSemaphoreGive(mutex);
}

