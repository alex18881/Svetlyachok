#include "endstop.h"

Endstop::Endstop(uint8_t pin, uint8_t mode, uint8_t debounceTime) {
	_pin = pin;
	_mode = mode;
	_debounceTime = debounceTime;
}

void Endstop::registerOnTtriggered(CallbackFunction fn) {
	pinMode(_pin, INPUT_PULLUP);
	cb = fn;
}

void Endstop::loop() {
	
	uint8_t val = digitalRead(_pin);

	bool triggered = val == LOW;

	if (val != prevVal)
		Serial.printf("-- Value changed to %d\n", val);

	prevVal = val;

	if (triggered != prevState ){ //&& checkTime < millis()) {
		prevState = triggered;
		
		if (triggered && cb != NULL)
			cb(val);

		//checkTime = millis() + _debounceTime;
	}

}