/*
  Listfiles

 This example shows how print out the files in a
 directory on a SD card

 The circuit:
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4 (for MKRZero SD: SDCARD_SS_PIN)

 created   Nov 2010
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe
 modified 2 Feb 2014
 by Scott Fitzgerald

 This example code is in the public domain.

 */
#define DEBUG

#include <EinkDisplay.h>

EinkDisplay disp;

void setup() {
  while (!SerialUSB) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  // Open serial communications and wait for port to open:
  SerialUSB.begin(57600);

  SerialUSB.print("Initializing e-ink display and SD...");

  if(disp.begin()) {
    SerialUSB.println("initialization done.");
  } else {
    SerialUSB.println("initialization failed.");
    while(1);
  }

  disp.printSdContent();

  disp.clearBuffer();
  SerialUSB.println("Buffer erased");
  disp.uploadImage("SMILEYS.BMP", 0, 0);
  SerialUSB.println("Image uploaded");
  disp.updateScreen();
  SerialUSB.println("Display updated");

  disp.clearBuffer();
  disp.uploadImage("EXAMPLE.BMP", 0, 0);
  disp.updateScreen();
}

void loop() {
  // nothing happens after setup finishes.
}
