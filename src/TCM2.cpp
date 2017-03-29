/*
TCM2lib - MPico TCS gen2 E-ink arduino driver
Copyright (C) 2016  OXullo Intersecans <x@brainrapers.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "TCM2.h"

#define U16_MSB_TO_U8(u16) ((u16 >> 8) & 0xff)
#define U16_LSB_TO_U8(u16) (u16 & 0xff)
#define U8_TO_U16_MSB(u8) ((u8 & 0xff) << 8)
#define U8_TO_U16_LSB(u8) (u8 & 0xff)

// CPOL=1, CPHA=1, MSB first, SS polarity=active low
SPISettings TCM2::spiSettings(TCM2_SPI_SPEED, MSBFIRST, SPI_MODE3);
SPIClass *mySPI;

TCM2::TCM2()
{
}

void TCM2::begin(SPIClass *mySPI_, uint8_t tc_busy_pin_, uint8_t tc_enable_pin_, uint8_t ss_pin_)
{
    mySPI = mySPI_;
  
    tc_busy_pin = tc_busy_pin_;
    tc_enable_pin = tc_enable_pin_;
    ss_pin = ss_pin_;
  
    //mySPI->begin();

    digitalWrite(ss_pin, HIGH);
    pinMode(ss_pin, OUTPUT);  

    // Necessary to prepare the clock for a falling edge
    mySPI->beginTransaction(spiSettings);
    mySPI->endTransaction();

    pinMode(tc_enable_pin, OUTPUT);
    digitalWrite(tc_enable_pin, LOW);
    
    pinMode(tc_busy_pin, INPUT);
    
    delay(100);
    SerialUSB.println("Waiting for busy line");
    busyWait();
}

void TCM2::end()
{
    pinMode(ss_pin, INPUT);
    pinMode(tc_enable_pin, INPUT);
}

TCM2Response TCM2::getDeviceInfo(uint8_t *buffer)
{
    return sendAndReadData(TCM2_CMD_GET_DEVICE_INFO, 0x01, 0, buffer);
}

TCM2Response TCM2::getDeviceId(uint8_t *buffer)
{
    return sendAndReadData(TCM2_CMD_GET_DEVICE_ID, 0x01, TCM2_LE_GET_DEVICE_ID, buffer);
}

TCM2Response TCM2::getSystemInfo(uint8_t *buffer)
{
    return sendAndReadData(TCM2_CMD_GET_SYSTEM_INFO, 0x01, 0, buffer);
}

TCM2Response TCM2::getSystemVersionCode(uint8_t *buffer)
{
    return sendAndReadData(TCM2_CMD_GET_SYSTEM_VERSION_CODE, 0x01, 0x10, buffer);
}

TCM2Response TCM2::getSensorData(uint8_t *buffer)
{
    return sendAndReadData(TCM2_CMD_GET_SENSOR_DATA, 0, TCM2_LE_GET_SENSOR_DATA, buffer);
}

TCM2Response TCM2::getTemperature(float *temperature)
{
    uint8_t buffer[2];
    TCM2Response res = getSensorData(buffer);

    if (res == TCM2_EP_SW_NORMAL_PROCESSING) {
        *temperature = TCM2_TEMPERATURE_LF_M * (float)buffer[1] + TCM2_TEMPERATURE_LF_P;
    }

    return res;
}

TCM2Response TCM2::uploadImageData(const uint8_t *data, uint8_t length, TCM2FramebufferSlot fb_slot)
{
    return sendCommand(TCM2_CMD_UPLOAD_IMAGE_DATA, fb_slot, length, (uint8_t *)data, INVERT_BLACK_WHITE);
}

TCM2Response TCM2::getImageData(uint8_t *buffer, uint8_t length, TCM2FramebufferSlot fb_slot)
{
    return sendAndReadData(TCM2_CMD_GET_IMAGE_DATA, fb_slot, length, buffer);
}

TCM2Response TCM2::getChecksum(uint16_t *checksum, TCM2FramebufferSlot fb_slot)
{
    uint8_t buffer[2];
    TCM2Response res = sendAndReadData(TCM2_CMD_GET_CHECKSUM, fb_slot, TCM2_LE_GET_CHECKSUM, buffer);

    if (res == TCM2_EP_SW_NORMAL_PROCESSING) {
        *checksum = U8_TO_U16_MSB(buffer[0]) | U8_TO_U16_LSB(buffer[1]);
    }

    return res;
}

TCM2Response TCM2::resetDataPointer()
{
    return sendCommand(TCM2_CMD_RESET_DATA_POINTER, 0);
}

TCM2Response TCM2::imageEraseFrameBuffer(TCM2FramebufferSlot fb_slot)
{
    return sendCommand(TCM2_CMD_IMAGE_ERASE_FRAME_BUFFER, fb_slot);
}

TCM2Response TCM2::uploadImageSetROI(uint16_t xmin, uint16_t ymin, uint16_t xmax, uint16_t ymax, TCM2FramebufferSlot fb_slot)
{
    // Note: TCS2-P102 supports framebuffer slot selection, passed as P2
    uint8_t buffer[TCM2_LC_UPLOAD_IMAGE_SET_ROI];

    buffer[0] = U16_MSB_TO_U8(xmin);
    buffer[1] = U16_LSB_TO_U8(xmin);
    buffer[2] = U16_MSB_TO_U8(xmax);
    buffer[3] = U16_LSB_TO_U8(xmax);
    buffer[4] = U16_MSB_TO_U8(ymin);
    buffer[5] = U16_LSB_TO_U8(ymin);
    buffer[6] = U16_MSB_TO_U8(ymax);
    buffer[7] = U16_LSB_TO_U8(ymax);

    return sendCommand(TCM2_CMD_UPLOAD_IMAGE_SET_ROI, fb_slot, TCM2_LC_UPLOAD_IMAGE_SET_ROI, buffer);
}

TCM2Response TCM2::uploadImageFixVal(const uint8_t *data, uint8_t length,
        TCM2FramebufferSlot fb_slot)
{
    return sendCommand(TCM2_CMD_UPLOAD_IMAGE_FIX_VAL, fb_slot, length, (uint8_t *)data);
}

TCM2Response TCM2::uploadImageCopySlots(TCM2FramebufferSlot fb_slot_dest,
        TCM2FramebufferSlot fb_slot_source)
{
    return sendCommand(TCM2_CMD_UPLOAD_IMAGE_COPY_SLOTS, fb_slot_dest,
            TCM2_LC_UPLOAD_IMAGE_COPY_SLOTS, (uint8_t *)&fb_slot_source);
}

TCM2Response TCM2::displayUpdate(TCM2FramebufferSlot fb_slot, TCM2DisplayUpdateMode mode)
{
    switch (mode) {
        case TCM2_DISPLAY_UPDATE_MODE_DEFAULT:
            return sendCommand(TCM2_CMD_DISPLAY_UPDATE_DEFAULT, fb_slot);
            break;

        case TCM2_DISPLAY_UPDATE_MODE_FLASHLESS:
            return sendCommand(TCM2_CMD_DISPLAY_UPDATE_FLASHLESS, fb_slot);
            break;

        case TCM2_DISPLAY_UPDATE_MODE_FLASHLESS_INVERTED:
            return sendCommand(TCM2_CMD_DISPLAY_UPDATE_FLASHLESS_INV, fb_slot);
            break;

        default:
            // Never reached
            return TCM2_EP_SW_GENERAL_ERROR;
    }
}

void TCM2::startTransmission()
{
    digitalWrite(ss_pin, LOW);
    delayMicroseconds(TCM2_SS_ASSERT_DELAY_US);
    mySPI->beginTransaction(spiSettings);
}

void TCM2::endTransmission()
{
    mySPI->endTransaction();
    delayMicroseconds(TCM2_SS_DEASSERT_DELAY_US);
    digitalWrite(ss_pin, HIGH);
}

void TCM2::busyWait()
{
    delayMicroseconds(TCM2_BUSY_WAIT_DELAY_US);
    while(digitalRead(tc_busy_pin) == LOW) {
        #ifdef DEBUG
        SerialUSB.print(".");
        delay(10);
        #endif
    };
    delayMicroseconds(TCM2_BUSY_RELEASE_DELAY_US);
}

TCM2Response TCM2::sendCommand(uint16_t ins_p1, uint8_t p2, uint8_t lc, uint8_t *data, bool invert)
{
    #ifdef DEBUG
    SerialUSB.print("INS=");
    SerialUSB.print(U16_MSB_TO_U8(ins_p1), HEX);
    SerialUSB.print(" P1=");
    SerialUSB.print(U16_LSB_TO_U8(ins_p1), HEX);
    SerialUSB.print(" P2=");
    SerialUSB.print(p2, HEX);
    SerialUSB.print(" Lc=");
    SerialUSB.print(lc, HEX);
    SerialUSB.print(": ");
    #endif

    startTransmission();
    mySPI->transfer(U16_MSB_TO_U8(ins_p1));
    mySPI->transfer(U16_LSB_TO_U8(ins_p1));
    mySPI->transfer(p2);

    if (lc) {
        mySPI->transfer(lc);
        for(int i=0;i<lc;i++) {
          if(invert) {
            mySPI->transfer(~data[i]);
          } else {
            mySPI->transfer(data[i]);
          }
        }
        //mySPI->transfer(data, lc);
    }
    endTransmission();
    busyWait();

    startTransmission();
    TCM2Response res = (mySPI->transfer(0) << 8) | mySPI->transfer(0);
    endTransmission();
    busyWait();

    #ifdef DEBUG
    if (res != TCM2_EP_SW_NORMAL_PROCESSING) {
        SerialUSB.print(" ERR=");
        SerialUSB.println(res, HEX);
    } else {
        SerialUSB.println("OK");
    }
    #endif

    #ifdef TCM2_APPLY_RESPONSE_READOUT_WORKAROUND
    // ErrataSheet_rA, solution 1
    delayMicroseconds(1200);
    #endif

    return res;
}

TCM2Response TCM2::sendCommand(uint16_t ins_p1, uint8_t p2, uint8_t lc, uint8_t *data) {
    return sendCommand(ins_p1, p2, lc, data, false);
}

TCM2Response TCM2::sendCommand(uint16_t ins_p1)
{
    return sendCommand(ins_p1, 0, 0, NULL, false);
}

TCM2Response TCM2::sendCommand(uint16_t ins_p1, uint8_t p2)
{
    return sendCommand(ins_p1, p2, 0, NULL, false);
}

TCM2Response TCM2::sendAndReadData(uint16_t ins_p1, uint8_t p2, uint8_t le, uint8_t *buffer)
{
    startTransmission();
    mySPI->transfer(U16_MSB_TO_U8(ins_p1));
    mySPI->transfer(U16_LSB_TO_U8(ins_p1));
    mySPI->transfer(p2);
    mySPI->transfer(le);
    endTransmission();
    busyWait();

    startTransmission();

    if (le) {
        // Fixed size response expected
        for (uint8_t i = 0 ; i < le ; ++i) {
            buffer[i] = mySPI->transfer(0);
        }
    } else {
        // Read until zero byte
        uint8_t i = 0;
        while (1) {
            char ch = mySPI->transfer(0);
            buffer[i++] = ch;

            if (ch == 0) {
                break;
            }
        }
    }

    TCM2Response res = U8_TO_U16_MSB(mySPI->transfer(0)) | U8_TO_U16_LSB(mySPI->transfer(0));
    endTransmission();
    busyWait();

    #ifdef TCM2_APPLY_RESPONSE_READOUT_WORKAROUND
    // ErrataSheet_rA, solution 1
    delayMicroseconds(1200);
    #endif

    return res;
}

void TCM2::dumpLinesStates()
{
    SerialUSB.print("SS=");
    SerialUSB.print(digitalRead(ss_pin));
    SerialUSB.print(" TC_ENA=");
    SerialUSB.print(digitalRead(tc_enable_pin));
    SerialUSB.print(" TC_BUSY=");
    SerialUSB.print(digitalRead(tc_busy_pin));
    SerialUSB.print(" CLK=");
    SerialUSB.print(digitalRead(13));
    SerialUSB.print(" MISO=");
    SerialUSB.print(digitalRead(11));
    SerialUSB.print(" MOSI=");
    SerialUSB.println(digitalRead(12));
}
