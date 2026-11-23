#include "Battery.h"

#include "Config.h"
#include "EventLog.h"

namespace {

// Napon Li-ion celije po procentu napunjenosti. Kriva nije linearna: celija
// najveci deo praznjenja provede oko 3,7 V pa naglo padne pri kraju.
struct BatteryPoint {
  uint16_t mv;
  uint8_t pct;
};

const BatteryPoint BATTERY_CURVE[] = {
    {4200, 100}, {4100, 92}, {4000, 84}, {3900, 74}, {3800, 62}, {3700, 48},
    {3600, 32},  {3500, 18}, {3400, 8},  {3300, 3},  {3000, 0},
};

int percentFromMillivolts(uint32_t mv) {
  const size_t n = sizeof(BATTERY_CURVE) / sizeof(BATTERY_CURVE[0]);
  if (mv >= BATTERY_CURVE[0].mv) return 100;
  if (mv <= BATTERY_CURVE[n - 1].mv) return 0;

  for (size_t i = 1; i < n; i++) {
    if (mv >= BATTERY_CURVE[i].mv) {
      const BatteryPoint& hi = BATTERY_CURVE[i - 1];
      const BatteryPoint& lo = BATTERY_CURVE[i];
      const uint32_t span = hi.mv - lo.mv;
      return lo.pct + (int)((uint32_t)(hi.pct - lo.pct) * (mv - lo.mv) / span);
    }
  }
  return 0;
}

}  // namespace

BatteryReading batteryRead() {
  // analogReadMilliVolts, ne analogRead: koristi kalibraciju iz eFuse-a. Uzorci
  // su razmaknuti da se usrednji i talasanje koje pravi pojacalo dok svira.
  uint32_t sum = 0;
  for (int i = 0; i < BATTERY_SAMPLES; i++) {
    sum += analogReadMilliVolts(PIN_BATTERY);
    delayMicroseconds(500);
  }

  const uint32_t atPin = sum / BATTERY_SAMPLES;
  const uint32_t mv = (uint32_t)(atPin * BATTERY_DIVIDER);
  const int pct = percentFromMillivolts(mv);

  logf("[batt] %lu mV, %d%%", (unsigned long)mv, pct);
  return {mv, pct};
}
