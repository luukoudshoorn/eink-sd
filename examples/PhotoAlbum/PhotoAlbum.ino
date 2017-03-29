/*
 PhotoAlbum

 Display the available monochrome bitmap images on the SD card as a slide show.

 The circuit:
 * SD card attached to SPI bus as follows:
 ** MOSI - pin D11
 ** MISO - pin D12
 ** CLK - pin D13
 ** /CS - pin D10
 * E-ink display attached to secondary SPI bus as follows:
 ** MOSI - pin A2
 ** MISO - pin A0
 ** SCK - pin A3
 ** /CS - pin D5
 ** /BUSY - pin D7
 ** /EN - pin D6
 ** VIN - 3V3
 ** VDDIN - 3V3
 
 This example code is in the public domain.

 */

//Uncomment the following lines if you want feedback over SerialUSB
//#define DEBUG
//#define debugSerial SerialUSB

#include <SD.h>
#include <EinkDisplay.h>

EinkDisplay disp;
File root;

int filesToDisplay = 0;

void setup() {
#ifdef DEBUG
  while (!debugSerial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  // Open serial communications and wait for port to open:
  debugSerial.begin(57600);

  debugSerial.print("Initializing e-ink display and SD...");
#endif

  disp.begin();
  
  if (!SD.begin()) {
#ifdef DEBUG
    debugSerial.println("SD initialization failed!");
#endif
    return;
  }
  
  root = SD.open("/");
  filesToDisplay = countFiles(root, false);
  root.rewindDirectory();

#ifdef DEBUG
  debugSerial.println("initialization done.");

  //Display all files
  printDirectory(root, 0);
  root.rewindDirectory();
#endif
}

void loop() {
  if(filesToDisplay <= 0) {
    // Nothing to do here
    return;
  }
  
  // Open next file
  File next = root.openNextFile();
  
  if(!next) {
    // End of directory
    root.rewindDirectory();
    next = root.openNextFile();
  }

  // Display the image
  disp.clearBuffer();
  disp.uploadImage(next, 0, 0);
  disp.updateScreen();
  
  // Wait 10 seconds before displaying the next image
  delay(10000);
  
  if(filesToDisplay == 1) {
    // There was only one file which has been displayed. Don't update.
    filesToDisplay = 0;
  }
}

int countFiles(File dir, bool recursive) {
  int count = 0;
  while(true) {
    File next = dir.openNextFile();
    if (! next) {
      // no more files
      return count;
    }
    if(next.isDirectory()) {
      if(recursive) {
        count += countFiles(next, true);
      }
    } else {
      count++;
    }
    next.close();
  }
}

#ifdef DEBUG
// Print each file name and return the number of files
int printDirectory(File dir, int numTabs) {
  int numFiles = 0;
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      debugSerial.print('\t');
    }
    debugSerial.print(entry.name());
    if (entry.isDirectory()) {
      debugSerial.println("/");
      numFiles += printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      debugSerial.print("\t\t");
      debugSerial.println(entry.size(), DEC);
      numFiles++;
    }
    entry.close();
  }
  return numFiles;
}
#endif