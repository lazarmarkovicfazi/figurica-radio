#pragma once
#include <Arduino.h>

// Napon i procenat iz istog ocitavanja.
struct BatteryReading {
  uint32_t mv;
  int pct;
};

// Meri celiju preko delitelja na PIN_BATTERY. Traje desetak milisekundi.
BatteryReading batteryRead();
