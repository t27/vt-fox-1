/*
This file is part of The Virginia Tech Amateur Radio Association's AMSAT FOX-1
experiment.

There are two main projects from which this code derives:

ChibiOS/RT 2.4.3
jpegant

ChibiOS/RT is unmodified, and retains its source structure in ./os/

jpegant is modified from its original version, and best efforts are made to
annotate the differences between the released jpegant and the contributions made
by VTARA.

All of the source code within this project is copyright (2012, 2013):

Joseph "Mitch" Davis, WQ3C, mitchd@vt.edu
Kevin Burns, KJ4SYL, kevinpd@vt.edu
Virginia Polytechnic Institute and State University

This entire project is licensed under the GNU Public License (GPL) Version 3:


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
#include "camera_iface.h"

#define CLK_DELAY 5
static const GPTConfig gpt3cfg = {
  1000000,    /* 32MHz timer clock.*/
  NULL    /* Timer callback.*/
};
//5 ticks = 100kHz bus timing

//Setup the SCCB controller
void setupSCCB(void){
  //SCL
  palSetPadMode(CAM_CTL_PORT, CAM_SCL, PAL_MODE_OUTPUT_PUSHPULL |
                                       PAL_STM32_OSPEED_LOWEST );
  //SDA
  palSetPadMode(CAM_CTL_PORT, CAM_SDA, PAL_MODE_OUTPUT_OPENDRAIN |
                                       PAL_STM32_OSPEED_LOWEST );
}

//Use this to turn on the timer
static void prepareTimer(void){
  gptStart(&GPTD3, &gpt3cfg);
  return;
}

//Don't forget to turn it off when we're done transferring data
static void deactivateTimer(void){
  gptStop(&GPTD3);
  return;
}

//Initiate SCCB start condition -- requires the timer
static void startCondition(void){
  chSysLockFromIsr();
  palWritePad(CAM_CTL_PORT, CAM_SDA, 1);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SDA, 0);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 0);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  chSysUnlockFromIsr();
  return;
}

//Release the SCCB -- requires the timer
static void stopCondition(void){
  chSysLockFromIsr();
  palWritePad(CAM_CTL_PORT, CAM_SDA, 0);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);
  palWritePad(CAM_CTL_PORT, CAM_SDA, 1);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  chSysLockFromIsr();
  return;
}

//Idle the SCCB
static void idleState(void){
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);
  palWritePad(CAM_CTL_PORT, CAM_SDA, 1);
  return;
}
 
//Write a byte, MSB first, with don't-care at the end -- requires the timer
static void writeByte(uint8_t byte){
  chSysLockFromIsr();
  //MSB Bit 7
  palWritePad(CAM_CTL_PORT, CAM_SCL, 0);
  palWritePad(CAM_CTL_PORT, CAM_SDA, (byte & 0x80)>>7);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);

  //Bit 6
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 0);
  palWritePad(CAM_CTL_PORT, CAM_SDA, (byte & 0x40)>>6);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);

  //Bit 5
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 0);
  palWritePad(CAM_CTL_PORT, CAM_SDA, (byte & 0x20)>>5);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);

  //Bit 4
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 0);
  palWritePad(CAM_CTL_PORT, CAM_SDA, (byte & 0x10)>>4);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);
  
  //Bit 3
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 0);
  palWritePad(CAM_CTL_PORT, CAM_SDA, (byte & 0x08)>>3);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);
 
  //Bit 2
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 0);
  palWritePad(CAM_CTL_PORT, CAM_SDA, (byte & 0x04)>>2);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);

  //Bit 1
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 0);
  palWritePad(CAM_CTL_PORT, CAM_SDA, (byte & 0x02)>>1);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);

  //Bit 0
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 0);
  palWritePad(CAM_CTL_PORT, CAM_SDA, (byte & 0x01) );
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);
  

  //9th Don't Care bit
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 0);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  //Deassert don't care
  palSetPadMode(CAM_CTL_PORT, CAM_SDA, PAL_MODE_INPUT );
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  chSysUnlockFromIsr();
  //Re-assert the SDA
  palSetPadMode(CAM_CTL_PORT, CAM_SDA, PAL_MODE_OUTPUT_OPENDRAIN |
                                       PAL_STM32_OSPEED_LOWEST );
  //Done with byte
  return;
}

//Write reg <= value using the three-phase transmission cycle in the SCCB
//datasheet
static void cameraWriteCycle( uint8_t reg, uint8_t value ){
  startCondition();
  writeByte(CAM_ADDR_W);
  writeByte(reg);
  writeByte(value);
  stopCondition();
  return;
}

static uint8_t readByte(void){ 
  chSysLockFromIsr();
  //Configure input pad 
  uint8_t byte = 0;
  palSetPadMode(CAM_CTL_PORT, CAM_SDA, PAL_MODE_INPUT );

  //MSB Bit 7
  palWritePad(CAM_CTL_PORT, CAM_SCL, 0);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);
  byte |= ( palReadPad(CAM_CTL_PORT, CAM_SDA) )<<7;
  gptPolledDelay(&GPTD3,CLK_DELAY);

  //Bit 6
  palWritePad(CAM_CTL_PORT, CAM_SCL, 0);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);
  byte |= ( palReadPad(CAM_CTL_PORT, CAM_SDA) )<<6;
  gptPolledDelay(&GPTD3,CLK_DELAY);

  //Bit 5
  palWritePad(CAM_CTL_PORT, CAM_SCL, 0);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);
  byte |= ( palReadPad(CAM_CTL_PORT, CAM_SDA) )<<5;
  gptPolledDelay(&GPTD3,CLK_DELAY);

  //Bit 4
  palWritePad(CAM_CTL_PORT, CAM_SCL, 0);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);
  byte |= ( palReadPad(CAM_CTL_PORT, CAM_SDA) )<<4;
  gptPolledDelay(&GPTD3,CLK_DELAY);

  //Bit 3
  palWritePad(CAM_CTL_PORT, CAM_SCL, 0);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);
  byte |= ( palReadPad(CAM_CTL_PORT, CAM_SDA) )<<3;
  gptPolledDelay(&GPTD3,CLK_DELAY);

  //Bit 2
  palWritePad(CAM_CTL_PORT, CAM_SCL, 0);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);
  byte |= ( palReadPad(CAM_CTL_PORT, CAM_SDA) )<<2;
  gptPolledDelay(&GPTD3,CLK_DELAY);

  //Bit 1
  palWritePad(CAM_CTL_PORT, CAM_SCL, 0);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);
  byte |= ( palReadPad(CAM_CTL_PORT, CAM_SDA) )<<1;
  gptPolledDelay(&GPTD3,CLK_DELAY);

  //Bit 0
  palWritePad(CAM_CTL_PORT, CAM_SCL, 0);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);
  byte |= ( palReadPad(CAM_CTL_PORT, CAM_SDA) );
  gptPolledDelay(&GPTD3,CLK_DELAY);

  //9th Don't Care Byte
  palWritePad(CAM_CTL_PORT, CAM_SCL, 0);
  //Dive to 1 per datasheet
  palSetPadMode(CAM_CTL_PORT, CAM_SDA, PAL_MODE_OUTPUT_OPENDRAIN |
                                       PAL_STM32_OSPEED_LOWEST );
  palWritePad(CAM_CTL_PORT, CAM_SDA, 1);  
  gptPolledDelay(&GPTD3,CLK_DELAY);
  palWritePad(CAM_CTL_PORT, CAM_SCL, 1);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  chSysUnlockFromIsr();
  return byte;
}
//Read reg => retval using two-phase write and read cycle -- requires the timer
static uint8_t cameraReadCycle( uint8_t reg ){
  
  uint8_t byte;
  
  startCondition();
  
  writeByte(CAM_ADDR_W);
  writeByte(reg);
  
  startCondition();
  
  writeByte(CAM_ADDR_R);
  gptPolledDelay(&GPTD3,CLK_DELAY);
  byte = readByte();
  
  stopCondition();
  return byte;
}
//Configure the camera pads
void setupCamPort(void){
  palSetPadMode(CAM_PORT, CAM_XCLK, PAL_MODE_ALTERNATE(0) | 
                                    PAL_STM32_OSPEED_HIGHEST);
  palSetPadMode(CAM_PORT, CAM_RESET, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(CAM_PORT, CAM_PWDN, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(CAM_PORT2, CAM_VSYNC_OUT, PAL_MODE_INPUT);
  palSetPadMode(CAM_PORT2, CAM_HREF_OUT, PAL_MODE_INPUT);
  palSetPadMode(FIFO_DATA_PORT, FIFO_D0, PAL_MODE_INPUT);
  palSetPadMode(FIFO_DATA_PORT, FIFO_D1, PAL_MODE_INPUT);
  palSetPadMode(FIFO_DATA_PORT, FIFO_D2, PAL_MODE_INPUT);
  palSetPadMode(FIFO_DATA_PORT, FIFO_D3, PAL_MODE_INPUT);
  palSetPadMode(FIFO_DATA_PORT, FIFO_D4, PAL_MODE_INPUT);
  palSetPadMode(FIFO_DATA_PORT, FIFO_D5, PAL_MODE_INPUT);
  palSetPadMode(FIFO_DATA_PORT, FIFO_D6, PAL_MODE_INPUT);
  palSetPadMode(FIFO_DATA_PORT, FIFO_D7, PAL_MODE_INPUT);
  palSetPadMode(FIFO_CTL_PORT, FIFO_WEN, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(FIFO_CTL_PORT, FIFO_RRST, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(FIFO_CTL_PORT, FIFO_OE, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(FIFO_CTL_PORT, FIFO_RCLK, PAL_MODE_OUTPUT_PUSHPULL);
  return;
}

//Configure the camera
msg_t configureCam(void){
  uint8_t rxbyte;
  idleState();
  //Poweron the camera
  wakeupCam();
  
  //cameraWriteCycle( CAM_CLKRC, 0x40 );
  //cameraWriteCycle( CAM_COM7, 0x00 );
  rxbyte = cameraReadCycle( 0x0A );
/*
  uint8_t tx_buf[32];
  uint8_t rx_buf[4];
  uint8_t address;
  //30 fps VGA YUV Mode
  tx_buf[0] = CAM_CLKRC;
  tx_buf[1] = 0x01;
  tx_buf[2] = CAM_COM7;
  tx_buf[3] = 0x00;
  tx_buf[4] = CAM_COM3;
  tx_buf[5] = 0x00;
  tx_buf[6] = CAM_COM14;
  tx_buf[7] = 0x00;
  tx_buf[8] = CAM_SCALING_XSC;
  tx_buf[9] = 0x3A;
  tx_buf[10] = CAM_SCALING_YSC;
  tx_buf[11] = 0x35;
  tx_buf[12] = CAM_SCALING_DCWCTR;
  tx_buf[13] = 0x11;
  tx_buf[14] = CAM_SCALING_PCK_DIV;
  tx_buf[15] = 0xF0;
  tx_buf[16] = CAM_SCALING_PCK_DELAY;
  tx_buf[17] = 0xA2;
*/

  chprintf((BaseChannel *)&SD1, "RX Byte %d\r\n", rxbyte );

  //Poweroff camera
  powerdownCam();
  if( rxbyte == 0x76 ){
    return RDY_OK;
  }else
    return -1;


}

void wakeupCam(){
  //palClearPad(CAM_PORT, CAM_PWDN);
}

void powerdownCam(){
  //palSetPad(CAM_PORT, CAM_PWDN);
}

msg_t cameraControlThread(void* arg){
  chprintf((BaseChannel *)&SD1, "Beginning camera control thread\r\n");
  setupSCCB();
  setupCamPort();
  while(TRUE){
    //chprintf((BaseChannel *)&SD1, "Inside camera thread\r\n");
    chThdSleepMilliseconds(2000);
    prepareTimer();
    msg_t success = configureCam();
    chprintf((BaseChannel *)&SD1, "Camera config status: %d\r\n", success);
    if( success == RDY_OK ){
       chprintf((BaseChannel *)&SD1, "Config TX OK\r\n");
    }else{
      chprintf((BaseChannel *)&SD1, "Unknown state returned\r\n");
    }
    deactivateTimer(); 
  }
}
