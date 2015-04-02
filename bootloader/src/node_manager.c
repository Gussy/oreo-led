#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "node_manager.h"
#include "twi_manager.h"

#define _NODE_UNINITIALIZED_STATION	0xff
static uint8_t _NODE_station = _NODE_UNINITIALIZED_STATION;

// return node station ID
// For SOLO, the ID scheme is zero-indexed, beginning
// with left-rear node, increasing counter-clockwise
uint8_t NODE_getId() {

    if (_NODE_station == _NODE_UNINITIALIZED_STATION) {
        SPCR    = 0x00; // disable SPI
        PCICR   = 0x00; // disable all pin interrupts
        
        // set PD6/PD7 as inputs (0 == input | 1 == output)
        DDRD = 0b00111111; 

        // turn off pullup resistor
        PORTD = 0x00;
		
		_delay_ms(200);

        cli();
        // wait for pullup to settle
        _delay_ms(15);

        _NODE_station = (PIND & 0b11000000) >> 6;
        sei();
    }

    return _NODE_station;

}
