#include <string.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/boot.h>
#include <util/delay.h>

#include "boot_manager.h"
#include "node_manager.h"
#include "twi_manager.h"

// Flash buffer
static uint8_t flash_buf[SPM_PAGESIZE];
static uint16_t flash_addr;
static uint8_t reply[TWI_SLR_BUFFER_SIZE];
	
static uint16_t app_version;

void BOOT_processBuffer(void)
{
	uint8_t size = TWI_Ptr;
	uint8_t* twiCommandBuffer = TWI_Buffer;
	
    // if command is new, re-parse
    // ensure valid length buffer
    // ensure pointer is valid
    if (BOOT_isCommandFresh && size > 0) {	
		// Clear the reply buffer
		//memset(reply, 0, TWI_SLR_BUFFER_SIZE);
			
		// Check the calculated CRC matches what was sent
		uint8_t masterXOR = twiCommandBuffer[size-1];
		if(masterXOR != TWI_BufferXOR) {
			debug_pulse(8);
			TWI_SetReply(reply, 0);
			BOOT_isCommandFresh = 0;
			return;
		}

		switch(twiCommandBuffer[0])
		{
			case BOOT_CMD_PING:
				reply[0] = (TWAR>>1);
				reply[1] = BOOT_CMD_PING;
				reply[2] = BOOT_CMD_PING_NONCE;
				reply[3] = TWI_BufferXOR;
				TWI_SetReply(reply, 4);
				break;
			
			case BOOT_CMD_BL_VER:
				reply[0] = (TWAR>>1);
				reply[1] = BOOT_CMD_BL_VER;
				reply[2] = BOOTLOADER_VERSION;
				reply[3] = TWI_BufferXOR;
				TWI_SetReply(reply, 4);
				break;
			
			case BOOT_CMD_WRITE_FLASH_A:
				// Send reply
				reply[0] = (TWAR>>1);
				reply[1] = BOOT_CMD_WRITE_FLASH_A;
				reply[2] = TWI_BufferXOR;
				TWI_SetReply(reply, 3);
				
				// Clear the 64 byte flash buffer and copy TWI buffer to flash buffer
				//memset(flash_buf, 0, SPM_PAGESIZE);
				memcpy(flash_buf, twiCommandBuffer+2, SPM_PAGESIZE/2);
				
				// Extract the page byte
				flash_addr = twiCommandBuffer[1]*SPM_PAGESIZE;
				break;

			case BOOT_CMD_WRITE_FLASH_B:
				// Send reply
				reply[0] = (TWAR>>1);
				reply[1] = BOOT_CMD_WRITE_FLASH_B;
				reply[2] = TWI_BufferXOR;
				TWI_SetReply(reply, 3);
			
				// Clear the 64 byte flash buffer and copy TWI buffer to flash buffer
				memcpy(flash_buf+(SPM_PAGESIZE/2), twiCommandBuffer+1, SPM_PAGESIZE/2);
				
				// Schedule a write flash operation
				BOOT_waitingToFlash = 1;
				break;

			case BOOT_CMD_FINALISE_FLASH:
				// Send reply
				reply[0] = (TWAR>>1);
				reply[1] = BOOT_CMD_FINALISE_FLASH;
				reply[2] = TWI_BufferXOR;
				TWI_SetReply(reply, 3);
				
				// Hold the version for writing later
				app_version = (twiCommandBuffer[1] << 8) | twiCommandBuffer[2];
			
				// Schedule a write flash operation
				BOOT_waitingToFinalise = 1;
				break;

			case BOOT_CMD_BOOT_APP:
				/* Checking the nonce doesn't work for some magical reason
				 * If we got this far we have essentially checked it via.
				 * the CRC check anyway, so we assume it's a valid boot command
				 */
				/*if(twiCommandBuffer[1] == BOOT_CMD_BOOT_NONCE) {
					BOOT_shouldBootApp = 1;
				}*/
				
				// Send a reply indicating if a boot is possible or not
				if(eeprom_read_word((uint16_t*)EEPROM_MAGIC_START) == EEPROM_MAGIC) {
					reply[0] = (TWAR>>1);
					reply[1] = BOOT_CMD_BOOT_APP;
					reply[2] = TWI_BufferXOR;
					TWI_SetReply(reply, 3);
					BOOT_shouldBootApp = 1;
				} else {
					TWI_SetReply(reply, 0);
				}
				
				break;
			
			default:
				// Send a reply containing the node address and the calculated XOR
				reply[0] = (TWAR>>1);
				reply[1] = TWI_BufferXOR;
				TWI_SetReply(reply, 2);
				break;
		}
    }

    // signal command has been parsed
    BOOT_isCommandFresh = 0;
}

void BOOT_write_flash_page(void)
{
	debug_pulse(4);
	// Clear the waiting to flash flag
	BOOT_waitingToFlash = 0;
	
	uint16_t pagestart = flash_addr;
	uint8_t *p = flash_buf;

	// Don't touch the bootloader section
	if(pagestart >= BOOTLOADER_START) {
		return;
	}
	
	// Preserve the interrupt vector table on the first page
	if(pagestart == INTVECT_PAGE_ADDRESS) {
		flash_buf[0] = pgm_read_byte(INTVECT_PAGE_ADDRESS + 0);
		flash_buf[1] = pgm_read_byte(INTVECT_PAGE_ADDRESS + 1);
		
		// Also erase the magic EEPROM key and app version
		eeprom_update_dword((uint32_t*)EEPROM_MAGIC_START, 0xFFFFFFFF);
		eeprom_busy_wait();
	}
	
	// Erase the page and wait
	boot_page_erase(pagestart);
	boot_spm_busy_wait();
	
	// Fill the flash page with the new data
	uint8_t i;
	for(i = 0; i < SPM_PAGESIZE; i+=2) {
		uint16_t data = *p++;
		data += (*p++) << 8;
		boot_page_fill(flash_addr+i, data);
	}

	// Commit the new page to flash
	boot_page_write(pagestart);
	boot_spm_busy_wait();
	debug_pulse(6);
}

void BOOT_finalise_flash(void)
{	
	// Also erase the magic EEPROM key since there's no going back now...
	eeprom_update_word((uint16_t*)EEPROM_MAGIC_START, EEPROM_MAGIC);
	eeprom_busy_wait();
	
	eeprom_update_word((uint16_t*)EEPROM_VERSION_START, app_version);
	eeprom_busy_wait();
	
	// Clear the waiting to finalise flag
	BOOT_waitingToFinalise = 0;
}