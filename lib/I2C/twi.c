/*
 twi.c - TWI/I2C library for Wiring & Arduino
 Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 Modified 2012 by Todd Krein (todd@krein.org) to implement repeated starts
 */

#include "CH58x_common.h"
#include "../usb/app_usb.h"

#include "twi.h"

/*
 * Function twi_init
 * Desc     readys twi pins and sets twi bitrate
 * Input    none
 * Output   none
 */
void twi_init(void) {
    GPIOB_ModeCfg( GPIO_Pin_13 | GPIO_Pin_12, GPIO_ModeIN_Floating);
    I2C_Init(I2C_Mode_I2C, TWI_FREQ, I2C_DutyCycle_16_9, I2C_Ack_Disable,
            I2C_AckAddr_7bit, TWI_ADDR);
}

/*
 * Function twi_reply
 * Desc     sends byte or readys receive line
 * Input    ack: byte indicating to ack or to nack
 * Output   none
 */
void twi_reply(uint8_t ack) {
    if (ack) {
        R16_I2C_CTRL1 |= RB_I2C_ACK;
    } else {
        R16_I2C_CTRL1 &= ~RB_I2C_ACK;
    }
}

/*
 * Function twi_start
 * Desc     relinquishes bus master status
 * Input    none
 * Output   none
 */
void twi_start(void) {
    R16_I2C_CTRL1 &= ~RB_I2C_STOP;
    R16_I2C_CTRL1 |= RB_I2C_START;
}

/*
 * Function twi_stop
 * Desc     relinquishes bus master status
 * Input    none
 * Output   none
 */
void twi_stop(void) {
    R16_I2C_CTRL1 &= ~RB_I2C_START;
    R16_I2C_CTRL1 |= RB_I2C_STOP;
}

void i2c_writeBytes(uint8_t devAddr, uint8_t regAddr, uint8_t* data,
        uint8_t length) {

    twi_start();
    uint32_t status = R16_I2C_STAR1;
    while(!(status&RB_I2C_SB)){status = R16_I2C_STAR1;}

    R16_I2C_DATAR=devAddr<<1;
    status = R16_I2C_STAR1;
    while(!(status & RB_I2C_TxE)){status = R16_I2C_STAR1;}
    // cdc_printf("devAddr sent %04x\n",status);

    R16_I2C_DATAR=regAddr;
    status = R16_I2C_STAR1;
    while(!(status & RB_I2C_TxE)){status = R16_I2C_STAR1;}
    // cdc_printf("regAddr sent %04x\n",status);
    for(uint8_t i=0;i<length;i++)
    {
        R16_I2C_DATAR=data[i];
        status = R16_I2C_STAR1|R16_I2C_STAR2<<16;
        while(!(status & RB_I2C_TxE)){status = R16_I2C_STAR1|R16_I2C_STAR2<<16;}
    }
    // cdc_printf("data sent %04x\n",status);

    twi_stop();
    // cdc_printf("writeByte Done\n");
}
void i2c_writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t data) {

    twi_start();
    uint32_t status = R16_I2C_STAR1;
    while(!(status&RB_I2C_SB)){status = R16_I2C_STAR1;}

    R16_I2C_DATAR=devAddr<<1;
    status = R16_I2C_STAR1;
    while(!(status & RB_I2C_TxE)){status = R16_I2C_STAR1;}
    // cdc_printf("devAddr sent %04x\n",status);

    R16_I2C_DATAR=regAddr;
    status = R16_I2C_STAR1;
    while(!(status & RB_I2C_TxE)){status = R16_I2C_STAR1;}
    // cdc_printf("regAddr sent %04x\n",status);

    R16_I2C_DATAR=data;
    status = R16_I2C_STAR1|R16_I2C_STAR2<<16;
    while(!(status & RB_I2C_TxE)){status = R16_I2C_STAR1|R16_I2C_STAR2<<16;}
    // cdc_printf("data sent %04x\n",status);

    twi_stop();
    // cdc_printf("writeByte Done\n");
}
uint8_t i2c_readBytes(uint8_t devAddr, uint8_t regAddr, uint8_t* data,
        uint8_t length) {
    uint32_t status;
    
    twi_start();
    while(!(R16_I2C_STAR1&RB_I2C_SB)){}
    
    R16_I2C_DATAR=devAddr<<1;
    status = R16_I2C_STAR1|R16_I2C_STAR2<<16;
    while(!(status & RB_I2C_TxE)){status = R16_I2C_STAR1|R16_I2C_STAR2<<16;}

    R16_I2C_DATAR=regAddr;
    status = R16_I2C_STAR1|R16_I2C_STAR2<<16;
    while(!(R16_I2C_STAR1 & RB_I2C_TxE)){status = R16_I2C_STAR1|R16_I2C_STAR2<<16;}

    twi_start();
    while(!(R16_I2C_STAR1&RB_I2C_SB)){}
    // cdc_printf("twi_restarted!\n");

    twi_reply(1);
    R16_I2C_DATAR=devAddr<<1|1;
    status = R16_I2C_STAR1|R16_I2C_STAR2<<16;
    while(!(status & RB_I2C_ADDR)){status = R16_I2C_STAR1|R16_I2C_STAR2<<16;}
    
    length--;
    for(uint8_t i=0; i<length; i++){
        status = R16_I2C_STAR1|R16_I2C_STAR2<<16;
        while(!(status & RB_I2C_RxNE)){status = R16_I2C_STAR1|R16_I2C_STAR2<<16;}
        data[i]=(uint8_t)R16_I2C_DATAR;
        // cdc_printf("dataReceived %04x %02x %d\n",status, data[i], i);
    }
    twi_reply(0);
    status = R16_I2C_STAR1|R16_I2C_STAR2<<16;
    while(!(status & RB_I2C_RxNE)){status = R16_I2C_STAR1|R16_I2C_STAR2<<16;}
    data[length++]=(uint8_t)R16_I2C_DATAR;
    // cdc_printf("readAddr sent %04x\n",status);
    twi_stop();
    
    return length;
}
uint8_t i2c_readByte(uint8_t devAddr, uint8_t regAddr) {
    uint32_t status;
    
    twi_start();
    while(!(R16_I2C_STAR1&RB_I2C_SB)){}
    
    R16_I2C_DATAR=devAddr<<1;
    status = R16_I2C_STAR1|R16_I2C_STAR2<<16;
    while(!(status & RB_I2C_TxE)){status = R16_I2C_STAR1|R16_I2C_STAR2<<16;}

    R16_I2C_DATAR=regAddr;
    status = R16_I2C_STAR1|R16_I2C_STAR2<<16;
    while(!(R16_I2C_STAR1 & RB_I2C_TxE)){status = R16_I2C_STAR1|R16_I2C_STAR2<<16;}

    twi_start();
    while(!(R16_I2C_STAR1&RB_I2C_SB)){}
    // cdc_printf("twi_started!\n");

    twi_reply(0);
    R16_I2C_DATAR=devAddr<<1|1;
    status = R16_I2C_STAR1|R16_I2C_STAR2<<16;
    while(!(status & RB_I2C_ADDR)){status = R16_I2C_STAR1|R16_I2C_STAR2<<16;}
    // cdc_printf("readAddr sent %04x\n",status);
    status = R16_I2C_STAR1|R16_I2C_STAR2<<16;
    while(!(status & RB_I2C_RxNE)){status = R16_I2C_STAR1|R16_I2C_STAR2<<16;}
    // cdc_printf("dataReceived %04x\n",status);
    twi_stop();
    uint8_t data=R16_I2C_DATAR;
    return data;
}
uint8_t i2c_checkDevice(uint8_t devAddr) {
    twi_start();
    while(!(R16_I2C_STAR1&RB_I2C_SB)){}

    R16_I2C_DATAR=devAddr<<1;
    uint16_t status = R16_I2C_STAR1;
    while(1){
        if(status & RB_I2C_AF) { R16_I2C_STAR1 &= ~RB_I2C_AF; twi_stop(); return 0;}
        if(status & (RB_I2C_TxE)) {twi_stop(); return 1;}
        

        status = R16_I2C_STAR1;
    }
}