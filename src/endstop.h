#ifndef Endstop_H
#include <Arduino.h>

#define Endstop_H

class Endstop {
	typedef void (*CallbackFunction) (uint8_t val);
	public:
		Endstop(uint8_t pin, uint8_t mode, uint8_t debounceTime);
		void registerOnTtriggered(CallbackFunction fn);
		void loop();

	private:
		bool prevState = false;
		uint8_t prevVal = 0;
		uint8_t _pin;
		uint8_t _mode = INPUT;
		unsigned long checkTime = 0;
		uint8_t _debounceTime;
		CallbackFunction cb = NULL;
};
#endif