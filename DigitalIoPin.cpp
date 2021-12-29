
#include "DigitalIoPin.h"
#include "board.h"
#include <cr_section_macros.h>


DigitalIoPin::DigitalIoPin(int port,int pin,bool input , bool pullup , bool invert) {
	// TODO Auto-generated constructor stub
	_input = input;
	_pin = pin;
	_port = port;
	_invert = invert;
	LPC_IOCON->PIO[_port][_pin] = (0x1 << 7);
	if(input){

		// Chip_IOCON_PinMuxSet
		//Set bits for Digital mode
		LPC_IOCON->PIO[_port][_pin] = (0x1 << 7);

		if(pullup){
			LPC_IOCON->PIO[_port][_pin] = LPC_IOCON->PIO[_port][_pin] | (0x2 << 3);
		} else {
			LPC_IOCON->PIO[_port][_pin] = LPC_IOCON->PIO[_port][_pin] | (0x1 << 3);
		}

		if(invert){
			LPC_IOCON->PIO[_port][_pin] = LPC_IOCON->PIO[_port][_pin] | (0x1 << 6);
		}
		//Chip_GPIO_SetPinDIRInput
		LPC_GPIO->DIR[_port] &= ~(1UL << _pin);
	} else {
		//Chip_GPIO_SetPinDIROutput
		LPC_GPIO->DIR[_port] |= 1UL << _pin;
	}
}

DigitalIoPin::~DigitalIoPin() {
	// TODO Auto-generated destructor stub
}

bool DigitalIoPin::read(){
	return (bool) LPC_GPIO->B[_port][_pin];
}

void  DigitalIoPin::write(bool value){
	// False puts voltage on the pin. A more intuitive way is negate the parameter input.
	if(_invert)LPC_GPIO->B[_port][_pin] = !value;
	else LPC_GPIO->B[_port][_pin] = value;
}

void DigitalIoPin::toggle(){
	LPC_GPIO->NOT[_port] = (1 << _pin);
}
