
#ifndef DIGITALIOPIN_H_
#define DIGITALIOPIN_H_

#include "DigitalIoPin.h"
#include "board.h"
#include <cr_section_macros.h>

class DigitalIoPin {
public:
	DigitalIoPin(int port,int pin,bool input = true, bool pullup = true, bool invert = false);
	DigitalIoPin(const DigitalIoPin &) = delete;
	virtual ~DigitalIoPin();
	bool read();
	void write(bool value);
	void toggle();
private:
	bool _input;
	int _port;
	int _pin;
	bool _invert;
};

#endif /* DIGITALIOPIN_H_ */
