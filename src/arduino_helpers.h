#ifndef ARDUINO_HELPERS_H_
#define ARDUINO_HELPERS_H_

#include "HeliOS.h"

#ifdef __cplusplus

  #if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_SAM) || defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_STM32) || \
  defined(ARDUINO_TEENSY_MICROMOD) || defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY41) || defined(ARDUINO_TEENSY36) || defined(ARDUINO_TEENSY35) || \
  defined(ARDUINO_TEENSY31) || defined(ARDUINO_TEENSY32) || defined(ARDUINO_TEENSY30) || defined(ARDUINO_TEENSYLC)
  #include <Arduino.h>

    String xByte2String(xSize size_, xByte *bytes_);

  #endif /* if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_SAM) ||
          * defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_ARCH_ESP8266) ||
          * defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_TEENSY_MICROMOD) ||
          * defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY41) ||
          * defined(ARDUINO_TEENSY36) || defined(ARDUINO_TEENSY35) ||
          * defined(ARDUINO_TEENSY31) || defined(ARDUINO_TEENSY32) ||
          * defined(ARDUINO_TEENSY30) || defined(ARDUINO_TEENSYLC) */

#endif /* ifdef __cplusplus */

#endif /* ifndef ARDUINO_HELPERS_H_ */