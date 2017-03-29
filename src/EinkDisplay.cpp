#include "EinkDisplay.h"

SPIClass *EINKSPI;
TCM2 tcm;

//2: MISO
//3: SCK
//4: MOSI
//5: CS
SPIClass SPI5(&sercom5, A0, A3, A2, SPI_PAD_0_SCK_1, SERCOM_RX_PAD_2);

uint8_t image_buffer[MAX_TRANSFER];

EinkDisplay::EinkDisplay()
{
}

bool EinkDisplay::begin() {
  SPI5.begin();
  
  pinPeripheral(A0, PIO_SERCOM_ALT);
  pinPeripheral(A3, PIO_SERCOM_ALT);
  pinPeripheral(A2, PIO_SERCOM_ALT);
  
  tcm.begin(&SPI5, TCM2_BUSY_PIN, TCM2_ENABLE_PIN, TCM2_SPI_CS);
  
  if (!SD.begin()) {
#ifdef DEBUG
    SerialUSB.println("initialization failed!");
#endif
    return false;
  }
  return true;
}

void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      SerialUSB.print('\t');
    }
    SerialUSB.print(entry.name());
    if (entry.isDirectory()) {
      SerialUSB.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      SerialUSB.print("\t\t");
      SerialUSB.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void EinkDisplay::printSdContent() {
  File dir = SD.open("/");
  printDirectory(dir, 1);
}

void EinkDisplay::uploadImage(const char *name, uint16_t x, uint16_t y) {
  //get data from SD
  File f = SD.open(name);
  if(!f) {
    return;
  }
  
  if(f.size() == 0) {
    f.close();
    return;
  }
  
  //--------- parse BMP header ------------
  
  //only allow bitmaps
  if(!(f.read() == 'B' && f.read() == 'M')) {
    f.close();
    return;
  }
  
  //data start
  f.seek(0x0A);
  uint32_t dataPos = f.read();
  dataPos |= (f.read())<<8;
  dataPos |= (f.read())<<16;
  dataPos |= (f.read())<<24;
  
  //width
  f.seek(0x12);
  uint16_t width = f.read();
  width |= (f.read())<<8;
  
  //height
  f.seek(0x16);
  uint16_t height = f.read();
  height |= (f.read())<<8;
  
  //check if planes == 1
  f.seek(0x1A);
  if(f.read() != 1) {
    f.close();
    return;
  }
  
  //check if bits per pixel == 1
  f.seek(0x1C);
  if(f.read() != 1) {
    f.close();
    return;
  }
  
  uint16_t lineWidth = width>>3;
  if(width%32 != 0) {
    //Bitmap data is padded at 4 bytes
    lineWidth += (4 - ((width%32)>>3));
  }
  
  uint8_t lineDataBytes = width>>3;
  if(width%8 != 0) {
    //Well that's unfortunate, but we have to send a multiple of 8 bits
#ifdef DEBUG
    SerialUSB.println("WARNING: Image width should be a multiple of 8 bits! Adding padding...");
#endif
    lineDataBytes++;
  }
  
  //data in bitmap is stored bottom line first
  //point to the start of the last line
  dataPos += lineWidth * (height - 1);

#ifdef DEBUG
  f.seek(0x22);
  uint32_t dataSize = f.read();
  dataSize |= (f.read())<<8;
  dataSize |= (f.read())<<16;
  dataSize |= (f.read())<<24;
  
  SerialUSB.println("Printing image");
  SerialUSB.print("Width: ");
  SerialUSB.println(width);
  SerialUSB.print("Height: ");
  SerialUSB.println(height);
  SerialUSB.print("lineWidth (bytes):");
  SerialUSB.println(lineWidth);
  SerialUSB.print("Line data bytes:");
  SerialUSB.println(lineDataBytes);
  SerialUSB.print("dataSize (bytes):");
  SerialUSB.println(dataSize);
  SerialUSB.print("Start of data:");
  SerialUSB.println(dataPos);
#endif
  
  //---------- upload image --------------
  tcm.uploadImageSetROI(x, y, x+(lineDataBytes<<3), y+height, FRAME_DEFAULT);
  
  for(uint16_t currentLine=0;currentLine<height;currentLine++) {
    f.seek(dataPos);
    
#ifdef DEBUG
    SerialUSB.println(currentLine);
#endif
    
    //always transfer one line at a time
    f.read(image_buffer, lineDataBytes);
    tcm.uploadImageData(image_buffer, lineDataBytes, FRAME_DEFAULT);
    
    dataPos -= lineWidth;
  }
}

void EinkDisplay::clearBuffer() {
  tcm.imageEraseFrameBuffer(FRAME_DEFAULT);
}

void EinkDisplay::updateScreen() {
  tcm.displayUpdate(FRAME_DEFAULT, TCM2_DISPLAY_UPDATE_MODE_DEFAULT);
  //start with clear, or ROI commands won't work (might have misread this part in datasheet, doesn't seem to make sense)
  clearBuffer();
  //populate with current image
  tcm.uploadImageCopySlots(FRAME_DEFAULT, FRAME_DISPLAYED);
}