# UC1617-Arduino-Library
Arduino library for a bufferless UC1617 128x88 I2C interface LCD display

The UC1617 display controller has graphics display memory (GDRAM) of 128x128 2-bit (4 grayscale) pixels. The GDRAM stores four 2-bit pixels in each addressable byte. So, the GDRAM address range is 16x128.

This library was written for an OLED display with 128x88 display pixels using an I2C interface. This I2C interface permits the reading of GDRAM, so individual pixels can be read and written.

Since GDRAM can be read, no display buffer is needed. Pixels are packed and unpacked when writing and reading.

The display used for this development was from eBay.
https://www.ebay.ca/itm/128x88-pixel-4-level-grayscale-I2C-LCD-UC1617-with-backlight-for-Arduino-NEW-/292654810960

The display was also available on https://www.hobbielektronika.hu/apro/apro_111585.html, be appears there no longer.
A copy of tThe zip bundle from that site is kept here. That bundle included an Arduino sketch which was used to develop this library.

