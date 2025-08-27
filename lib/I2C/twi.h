/*
  twi.h - TWI/I2C library for CH58x
  Copyright (c) 2025 Cheolwoo Lim.  All right reserved.

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
*/

#ifndef twi_h
#define twi_h

#include <inttypes.h>

#ifndef TWI_FREQ
#define TWI_FREQ 400000L
#endif

#ifndef TWI_ADDR
#define TWI_ADDR 0x53
#endif

void twi_init(void);
void twi_reply(uint8_t);
void twi_start(void);
void twi_stop(void);

void i2c_writeBytes(uint8_t devAddr, uint8_t regAddr, uint8_t* data, uint8_t length);
void i2c_writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t data);
uint8_t i2c_readBytes(uint8_t devAddr, uint8_t regAddr, uint8_t* data, uint8_t length);
uint8_t i2c_readByte(uint8_t devAddr, uint8_t regAddr);
uint8_t i2c_checkDevice(uint8_t devAddr);
#endif

