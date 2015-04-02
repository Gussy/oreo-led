#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/cpufunc.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <util/twi.h>
#include <util/delay.h>

#include "twi_manager.h"
#include "node_manager.h"
#include "boot_manager.h"

// the watchdog timer remains active even after a system reset 
//  (except a power-on condition), using the fastest prescaler 
//  value (approximately 15ms). It is therefore required to turn 
//  off the watchdog early during program startup
//  (taken from wdt.h, avrgcc)
uint8_t mcusr_mirror __attribute__ ((section (".noinit")));
void get_mcusr(void) __attribute__((naked)) __attribute__((section(".init3")));
void get_mcusr(void)
{
    mcusr_mirror = MCUSR;
    MCUSR = 0;
    wdt_disable();
}

// Hack to put the reset vector in the right place
// See: http://www.mikrocontroller.net/articles/Konzept_f%C3%BCr_einen_ATtiny-Bootloader_in_C
#define RJMP (0xC000U - 1) // opcode of RJMP minus offset 1
uint16_t boot_reset __attribute__((section(".bootreset"))) = RJMP + BOOTLOADER_START /  2 ;

static void (*jump_to_app)(void) = (void (*)(void))0x1C;

// Flash: fill page word by word
#define boot_program_page_fill(byteaddr, word) \
{\
    sreg = SREG; \
    cli (); \
    boot_page_fill ((uint32_t) (byteaddr), word); \
    SREG = sreg; \
}
 
// Flash: erase and write page
#define boot_program_page_erase_write(pageaddr) \
{\
    eeprom_busy_wait (); \
    sreg = SREG; \
    cli (); \
    boot_page_erase ((uint32_t) (pageaddr)); \
    boot_spm_busy_wait (); \
    boot_page_write ((uint32_t) (pageaddr)); \
    boot_spm_busy_wait (); \
    SREG = sreg; \
}

// main program entry point
int main(void)
{
	uint16_t rjmp;
	uint8_t idx;
	uint8_t sreg;
	
	// Get RJMP address (Reset) from the Boot Loader section and provided with OFFEST 0x1A00:
	rjmp = pgm_read_word(BOOTLOADER_START) + BOOTLOADER_START / 2;
	
	// Compare with reset address, if different:
	if(rjmp != pgm_read_word(0)) {
		// copy all vectors by "front" - taking into account the Offsets
		for(idx =  0; idx < _VECTORS_SIZE; idx +=  2) {
			rjmp = pgm_read_word(BOOTLOADER_START + idx) + BOOTLOADER_START/2;
			boot_page_fill((uint32_t)idx , rjmp) ;
		}
		
		// Fill the rest of the Page (64 bytes) with zeros
		while(idx < SPM_PAGESIZE) {
			boot_program_page_fill((uint32_t)idx , 0x0000);
			idx +=  2;
		}
		boot_program_page_erase_write(0x0000) ;
	}
	
    cli();
    // delay to acquire hardware pin settings
    // and node initialization
    uint8_t i;
    for (i=0; i<10; i++) {
        PORTB |= 0b00010000; 
        _delay_ms(5);
        PORTB &= ~0b00010000; 
        _delay_ms(5);
    }

    // init TWI node singleton with device ID
    TWI_init();

    // enable interrupts 
    sei();
	
    while(1) {
		// Process the TWI buffer
		BOOT_processBuffer();
		
		// Process a waiting flash write after TWI transactions
		if(!TWI_readIsBusy && BOOT_waitingToFlash)
			BOOT_write_flash_page();
			
		// Process a waiting finalise after TWI transactions
		if(!TWI_readIsBusy && BOOT_waitingToFinalise)
			BOOT_finalise_flash();
		
		// Boot the app if required and if the response is set
		if(!TWI_readIsBusy && BOOT_shouldBootApp) {
			_delay_ms(10);
			jump_to_app();
		}
    }
}
