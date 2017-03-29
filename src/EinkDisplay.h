#ifndef __EINKDISPLAY_H__
#define __EINKDISPLAY_H__

//Uncomment the next lines if you want more debug information shown over SerialUSB
//#define DEBUG
//#define debugSerial SerialUSB

#include <SPI.h>
#include <SD.h>

#include "TCM2.h"
#include "wiring_private.h"

#define TCM2_BUSY_PIN       7
#define TCM2_ENABLE_PIN     6
#define TCM2_SPI_CS         5

#define MAX_TRANSFER 0xFA

#define SCREENWIDTH 1024
#define SCREENHEIGHT 1280

#define FRAME_DEFAULT   0
#define FRAME_DISPLAYED -1
#define FRAME_TEMP      -2
#define FRAME_NEXT      -3

class EinkDisplay {
  public:
    EinkDisplay(void);
    
    void begin(void);
    void clearBuffer(void);
    bool uploadImage(File f, uint16_t x, uint16_t y);
    void updateScreen(void);
  private:
    TCM2FramebufferSlot displayed;
    TCM2FramebufferSlot next;
    TCM2FramebufferSlot temp;
};

#endif