// uc1617demo.ino
// 
// with Wu anti-aliased line drawing algorithm
// 
// 128x88 pixel (4-level grayscale) I2C LCD (UC1617) with backlight for Arduino NEW
//   https://www.ebay.ca/itm/128x88-pixel-4-level-grayscale-I2C-LCD-UC1617-with-backlight-for-Arduino-NEW-/292654810960
// 
// https://www.hobbielektronika.hu/apro/apro_111585.html
//   2020-01-11
//   [not found on 2020-01-17]
// 
// https://goo.gl/nJBeK9
//  --> https://www.dropbox.com/s/12ijqcoiyfvbq00/uc1617-lcd.zip
// 2020-01-17

// Wiring
// There are a number of devices on the I2C bus.
//   UC1617 LCD display controller
//   PCF8575 I2C I/O expander
//   CAT3626 LED driver
//  (BD1606 LED driver is an alternative to a CAT3626)
// 
// X1 - Power, I2C
//   1 +5V                  = 5V
//   2 SDA                  = SDA / A4
//   3 SCL                  = SCL / A5
//   4 GND                  = GND
//   5 INT - PCF8575        = 
//   6 GND                  = 
// 
// Only 9 pins of the PCF8575 are accessible.
// They are Port A(0) pin0,4~7 Port B(1) pins 4~7, and the Interrupt pin
// The pin modes listed below can (?) be changed by performing a read or a write.
// 
// X3 - PCF8575
//   1 +5V                  = 
//   2 P17 - Output         = 
//   3 P16 - Output         = 
//   4 P15 - Output         = 
//   5 P14 - Output         = 
//     P13 -
//     P12 -
//     P11 -
//     P10 -
//     P00 -
//     P01 -
//     P02 -
//     P03 -
//   6 P04 - Input          = 
//   7 P05 - Input          = 
//   8 P06 - Input          = 
//   9 P07 - Input          = 
//  10 GND                  = 
// 
// X4 - PCF8575
//   1 P00 - Input          = 
//   2 GND                  = 

// PCF8575 write example
// only pins 0, 4~7 of Port A are accessible
// only pins    4~7 of Port B are accessible
//   I2C_SendTwoBytes( PCF8575_ADDR, 0xF1, 0xF0 );  // Write to PCF8575: Port A --> 0xF1, Port B --> 0xF0

// UC1617 examples

// ----
// set fixed lines: T top lines are fixed, and B bottom lines are fixed
//                  T and B are even and may range from 0 to 0x1E (30)
//                  Y mirror mode = 0 (MY=0)
//   I2C_SendTwoBytes( UC1617_CADDR, 0x90, ( ( ( T >> 1 ) << 4 ) & 0x0F ) | ( ( B >> 1 ) & 0x0F ) );
// 
// ----
// scroll up N lines - only non-fixed lines are scrolled
//   I2C_SendTwoBytes( UC1617_CADDR, ( 0x40 | ( N & 0x0F ) ), ( 0x50 | ( N >> 4 ) & 0x07 ) );
// 
// ----
// invert display: I = 1 --> invert; I = 0 --> normal
//   I2C_SendOneByte( UC1617_CADDR, 0xA6 | ( I & 0x01 ) );
// 
// ----
// set display enable: E = 1 --> enable; I = 0 --> disable
//   I2C_SendOneByte( UC1617_CADDR, 0xAE | ( E & 0x01 ) );
// 
// Note! : When the internal DC-DC converter starts to operate and pump out current to VLCD, there will be an
// in-rush pulse current between VDD2 and VSS2 initially. To avoid this current pulse from causing potential
// harmful noise, do NOT issue any command or write any data to UC1617 for 5~10mS after setting DC[2] to 1.
// 
// ----
// set mapping: MY = 1 --> mirror Y; MY = 0 --> normal Y; - immediate effect, GDRAM is not affected
//              MX = 1 --> mirror X; MX = 0 --> normal X; - no immediate effect, display changes when written
//   I2C_SendOneByte( UC1617_CADDR, 0xC1 | ( ( MY << 2 ) & 0x04 ) | ( ( MX << 1 ) & 0x02 ) );
// 
// ----
// set N-line inversion: NIV = 1 --> enable inversion; NIV = 0 --> disable inversion
//                       XOR = 1 --> enable XOR; XOR = 0 --> disable XOR
//                       lines = 0 --> 9 lines; lines = 1 --> 13 lines; lines = 2 --> 17 lines; lines = 3 --> 23 lines
//   I2C_SendTwoBytes( UC1617_CADDR, 0xC8, ( lines & 0x03 ) | ( ( XOR << 2 ) & 0x04 ) | ( ( NIV << 3 ) & 0x08 ) );
// 
// ----
// set LCD gray shades: level = 0 --> set shade for pixel shade 1
//                          value = 0 --> shade  9; value = 1 --> shade 12; value = 2 --> shade 15; value = 3 --> shade 21
//                      level = 1 --> set shade for pixel shade 2
//                          value = 0 --> shade 15; value = 1 --> shade 21; value = 2 --> shade 24; value = 3 --> shade 27
//   sets gray shades for pixel shades 1 and 2
//   shade 0 is always  0
//   shade 3 is always 36
//   * shade 1 could be made darker than shade 1
//   I2C_SendOneByte( UC1617_CADDR, 0xB0 | ( ( level << 2 ) & 0x08 ) | ( value & 0x03 ) );
// 
// ----
// The window instructions set the RAM program boundaries.
// ?? The 128x88 display has:
//    startPage = 0x00,
//    endPage   = 0x1F (31) (128 pixels wide / 4 pixels/page = 32 pages)
//    startRow  = 0x00
//    endRow    = 0x57 (87)
// ?? what if start is greater than end?
// 
// set window start page
//   I2C_SendTwoBytes( UC1617_CADDR, 0xF4, ( startPage & 0x1F ) );
// set window end   page
//   I2C_SendTwoBytes( UC1617_CADDR, 0xF6, ( endPage   & 0x1F ) );
// set window start row
//   I2C_SendTwoBytes( UC1617_CADDR, 0xF5, ( startRow  & 0x7F ) );
// set window end   row
//   I2C_SendTwoBytes( UC1617_CADDR, 0xF7, ( endRow    & 0x7F ) );
// 
// enable window: EW = 0 --> disable window; EW = 1 --> enable window
// issue this instruction after changing any of the window bondaries
//   I2C_SendOneByte( UC1617_CADDR, 0xF8 | ( EW & 0x01 ) );
// 
// This command is to enable the Window Program Function. Window Program Enable should always be
// reset when changing the window program boundary and then set right before starting the new boundary
// program.
// 
// Window Program Function can be used to refresh the RAM data in a specified window of SRAM address.
// When window programming is enabled, the CA and RA increment and wrap around will be automatically
// adjusted, and therefore allow effective data update within the window.
// 
// The direction of Window Program will depend on these register settings:
//   wrap-around ~ WA          (AC[0])
//   increment direction ~ PID (AC[2])
//   auto-increment order      (AC[1])
//   mirror X ~ MX             (LC[1])
//   WA decides whether the program RAM address advances to next row / page_c after reaching the specified window page_c / row boundary.
//   PID controls the RAM address incrementing from WPP0 toward WPP1 (PID=0) or reverse the direction (PID=1).
//   Auto-increment order directs the RAM address increment vertically (AC[1]=1) or horizontally (AC[1]=0).
//   MX results the RAM page_c address incrementing from 127-WPC0 to 127-WPC1 (MX=1) or WPC0 to WPC1 (MX=0).
// ----


#include <Wire.h>

//#define _DEBUG_WULINE 1    // debug drawWULine()

#ifdef _DEBUG_WULINE
  #include <PrintEx.h>
  StreamEx serial = Serial;
  // serial.printf( "%17n\nFlash string [%p]\nA decimal:%f\nDEC: %ld\nHEX: %lx\n%17n\n",
  //                '-', F("Hi from flash!"), 3.1415f, 65000, 65000, '-' );
#endif

typedef unsigned long dword;

// the uc1617 has two I2C addresses, one for commands, the other for data
#define UC1617_CADDR 0x38  // command address
#define UC1617_DADDR 0x39  // data address

#define PCF8575_ADDR 0x20  // I2C I/O Expander [A0=0, A1=0, A2=0]
#define CAT3626_ADDR 0x66  // LED driver (backlight)


// --------

void I2C_SendOneByte( byte addr, byte b )
{
  Wire.beginTransmission( addr );
  Wire.write( b );
  Wire.endTransmission();
}

void I2C_SendTwoBytes( byte addr, byte b1, byte b2 )
{
  Wire.beginTransmission( addr );
  Wire.write( b1 );
  Wire.write( b2 );
  Wire.endTransmission();
}

// --------

// Status 1 definitions:
//   [ 1 | MX | MY | WA | DE | WS | MD | MS ]
//   MX: Status of register LC[1], mirror X.
//   MY: Status of register LC[2], mirror Y.
//   WA: Status of register AC[0]. Automatic page_c/row wrap around.
//   DE: Display enable flag. DE=1 when display is enabled
//   WS: MTP Command Succeeded
//   MD: MTP Option (1 - MTP version, 0 - non-MTP version)
//   MS: MTP action status
// Status 2 definitions:
//   [ Ver[0:1] | PMO[0:5] ]
//   Ver: IC Version Code, 00 ~ 11.
//   PMO[5:0]: PM offset value
// Status 3 definitions:
//   [ code[0:4] | PID[0:1] | MID[0:1] ]
//   Product Code: 1010b(Ah)
//   PID[1:0]: Provide access to ID pins connection status
//   MID[1:0]: LCM manufacturer’s configuration

void getStatus( byte& sb1, byte& sb2, byte& sb3 )
{
  Wire.requestFrom( UC1617_CADDR, 4 );  // 3 data + 1 dummy
  (void)Wire.read();
  sb1 = Wire.read();
  sb2 = Wire.read();
  sb3 = Wire.read();
}

// --------

void setPageAndRow( byte page, byte row )
{
  Wire.beginTransmission( UC1617_CADDR );
  Wire.write( 0x60 |   ( row        & 0x0F ) );  // set row address LSB
  Wire.write( 0x70 | ( ( row >> 4 ) & 0x07 ) );  // set row address MSB
  Wire.write( 0x00 |   ( page       & 0x1F ) );  // set column address
  Wire.endTransmission();
}

void set_XY( byte x, byte y )
{
  byte page = x / 4;                           // page
  byte q    = x % 4;                           // quad
  byte row  = y;

  setPageAndRow( page, row );
}

byte read_byte( byte page, byte row )
{
  byte d = 0x00;

  setPageAndRow( page, row );

  Wire.requestFrom( UC1617_DADDR, 2 );        // 1 data + 1 dummy
  (void)Wire.read();                          // dummy read
  d =   Wire.read();                          // read four 2-bit pixels

  return d;
}

void write_byte( byte page, byte row, byte b )
{
  setPageAndRow( page, row );

  Wire.beginTransmission( UC1617_DADDR );
  Wire.write( b );                            // write four 2-bit pixels
  Wire.endTransmission();
}

// --------

// GDRAM is organized as 32 pages wide by 88 rows high (likely 128 rows of GDRAM)
// each page consists of 4 columns
// the pixel at [x][y] is found in [x/4][y] as bits (x%4) << ( (x%4) << 1 )
// 
//           one page
//         +-------------------+
//    col:    0    1    2    3
//         +----+----+----+----+
//  row:   | 10 | 32 | 54 | 76 |
//         +----+----+----+----+
// 
//  note that the location of the pixels on the display are left to right
//  while within the byte they are right to left
// 
// r     = row
// p     = col / 4   (0 ~ ..)
// q     = col % 4   (0 ~  3)
// sh    = q << 1
// mask  = 0x03 << sh
// nask  = ~mask
// pixel = ( [r][p] & mask ) >> sh
// 
// 00 = white
// 01 = light gray
// 10 = dark gray
// 11 = black
// 

byte mask[4] = { 0b00000011, 0b00001100, 0b00110000, 0b11000000 };
byte nask[4] = { 0b11111100, 0b11110011, 0b11001111, 0b00111111 };  // == ~mask[]

// get the 2-bit pixel q within the byte b
byte get_p( byte b, byte q )
{
  q &= 0x3;                // q = 0~3
  byte d = b & mask[q];    // get the pixel q (bits q1 and q0)
  return ( d >> (q<<1) );  // shift the bits, returning a value 0~3
}

void set_p( byte& b, byte q, byte p )
{
  q &= 0x3;                // q = 0~3
  b &= nask[q];            // clear the pixel q-bits within b
  p &= 0x3;                // p = 0~3
  b |= ( p << (q<<1) );    // set the pixel value p within b
}

byte get_pixel( byte x, byte row )
{
  byte page = x / 4;                // page
  byte q    = x % 4;                // quad
  byte b = read_byte( page, row );  // read 4 pixels
  return get_p( b, q );
}

void put_pixel( byte x, byte row, byte p )
{
  byte page = x / 4;                // page
  byte q    = x % 4;                // quad
  byte b = read_byte( page, row );  // read 4 pixels
  set_p( b, q, p );                 // set 1 pixel
  write_byte( page, row, b );       // write back
}

// --------

void LCD_Fill( byte color )
{
  byte c = color & 0x03;
  byte b = ( c << 6 ) | ( c << 4 ) | ( c << 2 ) | c;

  setPageAndRow( 0, 0 );

  // Clear display - 88 rows x 128 columns
  for ( int i = 0; i < 88; i++ )                 // y = 0~87
  {
    Wire.beginTransmission( UC1617_DADDR );
    for ( int j = 0; j < 32; j++ )               // x = 32 x 8 bits = 256 bits / 2 bits/pixel = 128 pixels
      Wire.write( b );
    Wire.endTransmission();
  }
}

// --------

void LCD_Init()
{
  I2C_SendOneByte(  UC1617_CADDR, 0xE2 );        // system reset
  delay( 1000 );
  I2C_SendTwoBytes( UC1617_CADDR, 0x81, 0x34 );  // set Vbias = 0x34 (default = 0x4E)
//I2C_SendOneByte(  UC1617_CADDR, 0xC4 );        // mirror row
  LCD_Fill( 0x00 );
}

void LCD_TurnOn()
{
  I2C_SendOneByte( UC1617_CADDR, 0xAF );         // enable display, 4-shade mode, wake up
}

// --------

// Suchit
// 6 Nov 2007
// Antialiasing: Wu Algorithm
// https://www.codeproject.com/Articles/13360/Antialiasing-Wu-Algorithm
// WuLinesDlg.cpp
// DrawWuLine( CDC *pDC, int X0, int Y0, int X1, int Y1, COLORREF clrLine )
// 2020-03-02

void drawWuLine( int x0, int y0, int x1, int y1, byte colour )
{
#ifdef _DEBUG_WULINE
  serial.printf( "1: x0 = %3d, y0 = %3d, x1 = %3d, y1 = %3d\n", x0, y0, x1, y1 );
#endif

  // Make sure the line runs top to bottom
  if ( y0 > y1 )
  {
    int Temp = y0; y0 = y1; y1 = Temp;
        Temp = x0; x0 = x1; x1 = Temp;
  }

#ifdef _DEBUG_WULINE
  serial.printf( "2: x0 = %3d, y0 = %3d, x1 = %3d, y1 = %3d\n", x0, y0, x1, y1 );
#endif

  // Draw the initial pixel, which is always exactly intersected by the line and so needs no weighting
  put_pixel( x0, y0, colour );

#ifdef _DEBUG_WULINE
  serial.printf( "3: put_pixel():   x = %3d, y = %3d, colour = %1d\n", x0, y0, colour );
#endif

  int xs,
      dx = x1 - x0;
  if ( dx >= 0 )
  {
    xs = 1;
  }
  else
  {
    xs = -1;
    dx = -dx;  // make dx positive
  }
  int dy = y1 - y0;

#ifdef _DEBUG_WULINE
  serial.printf( "4: dx = %3d, dy = %3d, xs = %3d\n", dx, dy, xs );
#endif

  // Special cases: horizontal, vertical, and diagonal lines
  // they require no weighting because they go right through the center of every pixel

  if ( dy == 0 )  // Horizontal line
  {
    while ( dx-- != 0 )
    {
      x0 += xs;
      put_pixel( x0, y0, colour );
#ifdef _DEBUG_WULINE
      serial.printf( "H: put_pixel():   x = %3d, y = %3d, colour = %1d\n", x0, y0, colour );
#endif
    }
    return;
  }

  if ( dx == 0 )  // Vertical line
  {
    do
    {
      y0++;
      put_pixel( x0, y0, colour );
#ifdef _DEBUG_WULINE
      serial.printf( "V: put_pixel():   x = %3d, y = %3d, colour = %1d\n", x0, y0, colour );
#endif
    } while ( --dy != 0 );
    return;
  }

  if ( dx == dy )  // Diagonal line
  {
    do
    {
      x0 += xs;
      y0++;
      put_pixel( x0, y0, colour );
#ifdef _DEBUG_WULINE
      serial.printf( "D: put_pixel():   x = %3d, y = %3d, colour = %1d\n", x0, y0, colour );
#endif
    } while ( --dy != 0 );
    return;
  }

  // Line is not horizontal, diagonal, or vertical ...

  word errAdj;
  word errAccTemp;
  word errAcc = 0;  // initialize the line error accumulator to 0 - word so that it is automatically truncated
  byte w, w1;

  // Is this an X-major or Y-major line?

  if ( dy > dx )
  {
    // Y-major line
    // calculate 16-bit fixed-point fractional part of a pixel that X advances
    // each time Y advances 1 pixel, truncating the result so that we
    // won't overrun the endpoint along the X axis
    // errAdj = 0x0000 ~ 0x7FFF (0.0 ~ 0.5)
    errAdj = ( (dword)dx << 16 ) / (dword)dy;

#ifdef _DEBUG_WULINE
    serial.printf( "Y: errAdj = %08x, x = %3d, y = %3d\n", errAdj, x0, y0 );
#endif

    // Draw all pixels other than the first and last
    while ( --dy )
    {
      errAccTemp = errAcc;         // remember current accumulated error
      errAcc += errAdj;            // calculate error for next pixel
                                   //   errAcc is a 16-bit word, so when adding errAdj to errAcc,
      if ( errAcc <= errAccTemp )  //   it can never have a value > 0xFFFF -- it is truncated
      {                            //   and will therefore be less than the remembered Temp value
        x0 += xs;                  // advance the X coordinate
      }
      y0++;                        // Y-major, so always advance Y

#ifdef _DEBUG_WULINE
      serial.printf( "Y: errAcc = %08x, x = %3d, y = %3d\n", errAcc, x0, y0 );
#endif

      // The IntensityBits most significant bits of errAcc give us the intensity weighting
      // for this pixel, and the complement of the weighting for the paired pixel
      w  = errAcc >> 14;
      w1 = w ^ 3;

      put_pixel( x0,      y0, w1 );  // LCD, so swap w and w1
      put_pixel( x0 + xs, y0, w );
#ifdef _DEBUG_WULINE
      serial.printf( "Y: put_pixel():   x = %3d, y = %3d, w1 = %3d\n", x0,    y0, w1 );
      serial.printf( "Y: put_pixel():   x = %3d, y = %3d, w  = %3d\n", x0+xs, y0, w );
#endif
    }
  }
  else
  {
    // X-major line
    // calculate 16-bit fixed-point fractional part of a pixel that Y advances
    // each time X advances 1 pixel, truncating the result to avoid overrunning
    // the endpoint along the X axis
    errAdj = ( (dword)dy << 16 ) / (dword)dx;

#ifdef _DEBUG_WULINE
      serial.printf( "X: errAdj = %08x, x = %3d, y = %3d\n", errAdj, x0, y0 );
#endif

    // Draw all pixels other than the first and last
    while ( --dx )
    {
      errAccTemp = errAcc;         // remember current accumulated error
      errAcc += errAdj;            // calculate error for next pixel
                                   //   errAcc is a 16-bit word, so when adding errAdj to errAcc,
      if ( errAcc <= errAccTemp )  //   it can never have a value > 0xFFFF -- it is truncated
      {                            //   and will therefore be less than the remembered Temp value
        y0++;                      // advance the Y coordinate
      }
      x0 += xs;                    // X-major, so always advance X

#ifdef _DEBUG_WULINE
      serial.printf( "X: errAcc = %08x, x = %3d, y = %3d\n", errAcc, x0, y0 );
#endif

      // The IntensityBits most significant bits of errAcc give us the intensity weighting
      // for this pixel, and the complement of the weighting for the paired pixel
      w  = errAcc >> 14;
      w1 = w ^ 3;

      put_pixel( x0, y0,     w1 );  // LCD, so swap w and w1
      put_pixel( x0, y0 + 1, w );
#ifdef _DEBUG_WULINE
      serial.printf( "X: put_pixel():   x = %3d, y = %3d, w1 = %3d\n", x0, y0,   w1 );
      serial.printf( "X: put_pixel():   x = %3d, y = %3d, w  = %3d\n", x0, y0+1, w );
#endif
    }
  }

  // Draw the final pixel, which is always exactly intersected by the line and so needs no weighting
  put_pixel( x1, y1, colour );
#ifdef _DEBUG_WULINE
  serial.printf( "5: put_pixel():   x = %3d, y = %3d, colour = %1d\n\n", x1, y1, colour );
#endif
}

// --------

// setup

void setup()
{
#ifdef _DEBUG_WULINE
  Serial.begin( 38400 );
  serial.printf( "uc1617t: drawWuLine()\n" );
#endif

  Wire.begin();

  I2C_SendTwoBytes( CAT3626_ADDR, 0x01, 0x17 );  // set backlight LED current - Reg B  --> 0x27 (what are Reg A and Reg C set to?)
  I2C_SendTwoBytes( CAT3626_ADDR, 0x03, 0x3F );  // turn on backlight LED     - Reg En --> 0x3F - all 6 drivers are enabled

  LCD_Init();
  LCD_TurnOn();
  delay( 100 );  // required delay (5~10 mS) after turning LCD on

  LCD_Fill( 0x00 );

  randomSeed( analogRead( 0 ) );
}

// --------

// loop

byte p4 = 0x00;
byte p4r;

byte times4( byte b, byte q )
{
  byte p = get_p( b, q );
  byte d = 0x00;
  d = ( p << 6 ) | ( p << 4 ) | ( p << 2 ) | p;
  return d;
}

byte x0, y0, x1, y1;

boolean flip = true;

void loop()
{
  LCD_Fill( 0x00 );

  if ( flip )
  {
    for ( int r = 0; r < 32; r++ )
    {
      write_byte( 2, 8+2*r, p4++ );
      delay( 50 );
    }
    delay( 250 );
    for ( int r = 0; r < 32; r++ )
    {
      p4r = read_byte( 2, 8+2*r );
      write_byte(  4, 8+2*r, times4( p4r, 0 ) );
      write_byte(  6, 8+2*r, times4( p4r, 1 ) );
      write_byte(  8, 8+2*r, times4( p4r, 2 ) );
      write_byte( 10, 8+2*r, times4( p4r, 3 ) );
      delay( 50 );
    }
  }
  else
  {
    int lines = random( 8 ) + 2;
    for ( int l = 0; l < lines; l++ )
    {
      x0 = random( 120 ) + 4;
      y0 = random(  80 ) + 4;
      x1 = random( 120 ) + 4;
      y1 = random(  80 ) + 4;
      drawWuLine( x0, y0, x1, y1, 0x3 );
      delay( 500 );
    }
  }
  flip = !flip;
  delay( 3000 );
}

// --fin--
