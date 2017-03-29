#ifndef __EINKDISPLAY_H__
#define __EINKDISPLAY_H__

#define DEBUG

#include <SPI.h>
#include <SD.h>

#include "TCM2.h"
#include "wiring_private.h"

#define TCM2_BUSY_PIN       7
#define TCM2_ENABLE_PIN     6
#define TCM2_SPI_CS         5

#define MAX_TRANSFER 0xFA

#define FRAME_DEFAULT   0
#define FRAME_DISPLAYED -1
#define FRAME_TEMP      -2
#define FRAME_NEXT      -3

class EinkDisplay {
  public:
    EinkDisplay(void);
    
    bool begin(void);
    void printSdContent(void);
    void clearBuffer(void);
    void uploadImage(const char *name, uint16_t x, uint16_t y);
    void updateScreen(void);
};

#endif