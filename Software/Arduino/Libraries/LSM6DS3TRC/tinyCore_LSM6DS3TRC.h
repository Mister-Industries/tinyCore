/*!
 *  @file tinyCore_LSM6DSL.h
 *
 * 	I2C Driver for the tinyCore LSM6DSL 6-DoF Accelerometer and Gyroscope
 *library
 *
 * 	This is a library for the tinyCore LSM6DSL breakout:
 * 	https://www.mr.industries/
 *
 * 
 *  This code is based on Adafruit's LSM6DS library:
 * 	Adafruit invests time and resources providing this open source code, please
 *  support Adafruit and open-source hardware by purchasing products from them!
 *
 *	BSD license (see license.txt)
 */

#ifndef _TINYCORE_LSM6DS3TRC_H
#define _TINYCORE_LSM6DS3TRC_H

#include "Adafruit_LSM6DS.h"

#define LSM6DS3TRC_CHIP_ID 0x69 ///< LSM6DSL default device id from WHOAMI

#define LSM6DS3TRC_MASTER_CONFIG 0x1A ///< I2C Master config

/*!
 *    @brief  Class that stores state and functions for interacting with
 *            the LSM6DS3TRC
 */
class tinyCore_LSM6DS3TRC : public Adafruit_LSM6DS {
public:
  tinyCore_LSM6DS3TRC();
  ~tinyCore_LSM6DS3TRC(){};

  void enableI2CMasterPullups(bool enable_pullups);
  void enablePedometer(bool enable);

private:
  bool _init(int32_t sensor_id);
};

#endif
