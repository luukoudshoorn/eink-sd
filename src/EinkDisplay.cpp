#include "EinkDisplay.h"

SPIClass *EINKSPI;
TCM2 tcm;

//A0: MISO
//A3: SCK
//A2: MOSI
//D5: CS
SPIClass SPI5(&sercom5, A0, A3, A2, SPI_PAD_0_SCK_1, SERCOM_RX_PAD_2);

const uint8_t white = 0x00;

uint8_t image_buffer[MAX_TRANSFER];

EinkDisplay::EinkDisplay()
{
}

void EinkDisplay::begin() {
  SPI5.begin();
  
  pinPeripheral(A0, PIO_SERCOM_ALT);
  pinPeripheral(A3, PIO_SERCOM_ALT);
  pinPeripheral(A2, PIO_SERCOM_ALT);
  
  tcm.begin(&SPI5, TCM2_BUSY_PIN, TCM2_ENABLE_PIN, TCM2_SPI_CS);
  
  displayed = 1;
  next = 2;
  temp = 3;
  
  //current slot might be 2, so this command could fail
  tcm.imageEraseFrameBuffer(temp);
  tcm.uploadImageFixVal(&white, 1, temp);
  tcm.displayUpdate(temp, TCM2_DISPLAY_UPDATE_MODE_FLASHLESS);
  //display is now forced to slot 3, allowing slot 1 to be erased an shown
  tcm.imageEraseFrameBuffer(displayed);
  tcm.uploadImageFixVal(&white, 1, displayed);
  tcm.displayUpdate(displayed);
  //display now displays slot 1, allowing slot 2 and 3 to be erased
  tcm.imageEraseFrameBuffer(next);
  tcm.uploadImageFixVal(&white, 1, next);
  tcm.imageEraseFrameBuffer(temp);
  tcm.uploadImageFixVal(&white, 1, temp);
}

bool EinkDisplay::uploadImage(File f, uint16_t x, uint16_t y) {
  if(!f) {
    return false;
  }
  
  if(f.size() == 0) {
    return false;
  }
  
  //--------- parse BMP header ------------
  
  //only allow bitmaps
  if(!(f.read() == 'B' && f.read() == 'M')) {
    return false;
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
    return false;
  }
  
  //check if bits per pixel == 1
  f.seek(0x1C);
  if(f.read() != 1) {
    return false;
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
    debugSerial.println("WARNING: Image width should be a multiple of 8 bits! Adding padding...");
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
  
  debugSerial.println("Printing image");
  debugSerial.print("Width: ");
  debugSerial.println(width);
  debugSerial.print("Height: ");
  debugSerial.println(height);
  debugSerial.print("lineWidth (bytes):");
  debugSerial.println(lineWidth);
  debugSerial.print("Line data bytes:");
  debugSerial.println(lineDataBytes);
  debugSerial.print("dataSize (bytes):");
  debugSerial.println(dataSize);
  debugSerial.print("Start of data:");
  debugSerial.println(dataPos);
#endif
  
  uint16_t x2 = x+(lineDataBytes<<3);
  uint16_t y2 = y+height;
  
  //---------- upload image --------------
  
  //divide image in 5 non-overlapping areas
  //                   v---SCREENWIDTH
  //  0---x--------x2--+ 
  //  |                |
  //  |        1       |
  //  |                |
  //  y---+--------+---+
  //  | 2 | 3(ROI) | 4 |
  //  y2--+--------+---+
  //  |                |
  //  |        5       |
  //  |                |
  //  +--SCREENHEIGHT--+
  
  tcm.imageEraseFrameBuffer(temp);
  if(y > 0) {
    // Only if image is not on top edge
    tcm.uploadImageSetROI(0, 0, SCREENWIDTH, y, temp);
    tcm.uploadImageCopySlots(temp, next);
  }
  if(x > 0) {
    // Only if image is not on left edge
    tcm.uploadImageSetROI(0, y, x, y2, temp);
    tcm.uploadImageCopySlots(temp, next);
  }
  if(x2 < SCREENWIDTH) {
    // Only if image is not on right edge
    tcm.uploadImageSetROI(x2, y, SCREENWIDTH, y2, temp);
    tcm.uploadImageCopySlots(temp, next);
  }
  if(y2 < SCREENHEIGHT) {
    // Only if image is not on bottom edge
    tcm.uploadImageSetROI(0, y2, SCREENWIDTH, SCREENHEIGHT, temp);
    tcm.uploadImageCopySlots(temp, next);
  }
  tcm.uploadImageSetROI(x, y, x2, y2, temp);
  
  for(uint16_t currentLine=0;currentLine<height;currentLine++) {
    f.seek(dataPos);
    
#ifdef DEBUG
    debugSerial.println(currentLine);
#endif
    
    //always transfer one line at a time
    f.read(image_buffer, lineDataBytes);
    tcm.uploadImageData(image_buffer, lineDataBytes, temp);
    
    dataPos -= lineWidth;
  }
  
  tcm.imageEraseFrameBuffer(next);
  tcm.uploadImageCopySlots(next, temp);
  
  return true;
}

void EinkDisplay::clearBuffer() {
  tcm.imageEraseFrameBuffer(next);
  //default to white
  tcm.uploadImageFixVal(&white, 1, next);
}

void EinkDisplay::updateScreen() {
  tcm.displayUpdate(next, TCM2_DISPLAY_UPDATE_MODE_DEFAULT);
  TCM2FramebufferSlot oldNext = next;
  next = temp;
  temp = displayed;
  displayed = oldNext;
  clearBuffer();
  //start next image with copy of this image
  tcm.uploadImageCopySlots(next, displayed);
}