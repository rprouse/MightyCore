/***********************************************************************|
| Optiboot Flash read/write                                             |
|                                                                       |
| Read_write_without_buffer.ino                                         |
|                                                                       |
| A library for interfacing with Optiboot Flash's write functionality   |
| Developed in 2021 by MCUdude                                          |
| https://github.com/MCUdude/                                           |
|                                                                       |
| In this low-level example we write 16-bit values to one flash page,   |
| and 8-bit values to another page. What's different about this         |
| example is that we do this without using a RAM buffer where the       |
| contents are stored before writing to flash. Instead, the internal,   |
| temporary buffer is used. By doing this we reduce RAM usage, but it   |
| is not nearly as user friendly as using the Flash library.            |
|                                                                       |
| IMPORTANT THINGS:                                                     |
| - All flash content gets erased after each upload cycle               |
| - Allocated flash space must be page aligned (it is in this example)  |
| - Writing to EEPROM destroys temporary buffer so make sure you call   |
|   optiboot_page_write before reusing the buffer or using EEPROM       |
| - You can write only once into one location of temporary buffer       |
|***********************************************************************/


#include <optiboot.h>

// Workaround for devices that has 64kiB flash or less
#ifndef pgm_read_byte_far
#define pgm_read_byte_far pgm_read_byte_near
#endif
#ifndef pgm_get_far_address
#define pgm_get_far_address
#endif


// Allocate one flash pages for storing data. If you want to allocate space in the high progmem (>64kiB)
// You can replace PROGMEM with PROGMEM1
#define NUMBER_OF_PAGES 2
const uint8_t flashSpace[SPM_PAGESIZE * NUMBER_OF_PAGES] __attribute__((aligned(SPM_PAGESIZE))) PROGMEM = {};


// Function for writing 16-bit integers to a flash page
void flash_write_int(uint32_t base_addr, uint16_t offset_addr, int16_t data)
{
  // Start by erasing the flash page before we start writing to the buffer
  if ((offset_addr & 0xFF) == 0)
    optiboot_page_erase(base_addr + (offset_addr & 0xFF00));

  // Write the 16-bit value to the buffer
  optiboot_page_fill(base_addr + offset_addr, data);

  // Write the buffer to flash when the buffer is full
  if ((offset_addr & 0xFF) == 0xFE)
    optiboot_page_write(base_addr + (offset_addr & 0xFF00));
}


// Function to write bytes to a flash page
void flash_write_byte(uint32_t base_addr, uint16_t offset_addr, uint8_t data)
{
  static uint8_t low_byte = 0;

  if ((offset_addr & 0xFF) == 0)
    optiboot_page_erase(base_addr + (offset_addr & 0xFF00));

  if (!(offset_addr & 0x01)) // Address is 2, 4, 6 etc.
    low_byte = data;
  else                       // Address is 1, 3, 5 etc.
    optiboot_page_fill(base_addr + offset_addr, (data << 8) | low_byte);

  if ((offset_addr & 0xFF) == 0xFE)
    optiboot_page_write(base_addr + (offset_addr & 0xFF00));
}


// Function to force a flash page write operation
void flash_end_write(uint32_t base_addr, uint16_t offset_addr)
{
  // Write the buffer to flash if there are any contents in the buffer
  if ((offset_addr & 0xFF) != 0x00)
    optiboot_page_write(base_addr + (offset_addr & 0xFF00));
}


void setup()
{
  delay(2000);
  Serial.begin(9600);

  static uint16_t addr = 0;

  Serial.print(F("Filling up flash page 0 with 16-bit values...\n"));
  // Fill the first flash page (page 0) with 16-bit values (0x100 to 0x01FF)
  for(uint8_t data = 0; data < (SPM_PAGESIZE / 2); data++)
  {
    flash_write_int(pgm_get_far_address(flashSpace), addr, data + 0x0100); // Write data
    addr += 2; // Increase memory address by two since we're writing 16-bit values
  }
  // Force an end write in case it hasn't already been done in flash_write_int
  flash_end_write(pgm_get_far_address(flashSpace), --addr);


  Serial.print(F("Filling up flash page 1 with 8-bit values...\n"));
  // Fill the second flash page (page 1) with 0-bit values (0x00 to 0x0FF)
  for(uint16_t data = 0; data < SPM_PAGESIZE; data++)
  {
    addr++; // Increase memory address by one since we're writing 8-bit values
    flash_write_byte(pgm_get_far_address(flashSpace), addr, data); // Write data
  }
  // Force an end write in case it hasn't already been done in flash_write_byte
  flash_end_write(pgm_get_far_address(flashSpace), addr);


  Serial.print(F("Flash pages filled. Reading back their content.\nPage 0:\n"));
  for(uint16_t i = 0; i < SPM_PAGESIZE; i += 2)
    Serial.printf(F("Flash mem addr: 0x%05lx, content: 0x%04x\n"), pgm_get_far_address(flashSpace) + i, (pgm_read_byte_far(pgm_get_far_address(flashSpace) + i + 1) << 8) + (pgm_read_byte_far(pgm_get_far_address(flashSpace) + i)));

  Serial.println(F("\n\nPage 1:"));
  for(uint16_t i = 0; i < SPM_PAGESIZE; i++)
    Serial.printf(F("Flash mem addr: 0x%05lx, content: 0x%02x\n"), pgm_get_far_address(flashSpace) + SPM_PAGESIZE + i, pgm_read_byte_far(pgm_get_far_address(flashSpace) + SPM_PAGESIZE + i));
}


void loop()
{

}
