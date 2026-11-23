#pragma once
#include <Arduino.h>
#include <string.h>  // memcmp

struct TagUid {
  uint8_t bytes[10] = {};
  uint8_t len = 0;

  bool operator==(const TagUid& o) const {
    return len == o.len && memcmp(bytes, o.bytes, len) == 0;
  }
  bool operator!=(const TagUid& o) const { return !(*this == o); }
};

// Figurica na lezistu, preko RC522. Javlja dogadjaje, ne stanje: Placed kad
// se pojavi figurica (ili kad se jedna zameni drugom), Removed kad je nema.
class TagReader {
public:
  enum class Event { None, Placed, Removed };

  // false ako RC522 ne odgovara na SPI-ju.
  bool begin();

  // Registar verzije RC522: 0x91/0x92 original, 0x88/0xB2 kopije, 0x00 i
  // 0xFF znace da na SPI-ju niko ne odgovara. Cita se uzivo, za dijagnostiku.
  uint8_t version();

  // Zove se iz svakog prolaza petlje; sam drzi ritam od TAG_POLL_MS.
  Event poll();

  bool present() const { return _present; }
  const TagUid& uid() const { return _uid; }

  // Promasena pitanja dok je figurica stajala, od proslog poziva: koliko ih je
  // bilo i najduzi niz zaredom. Pokazuje smetnje i kad jos ne izazovu pauzu.
  void takeMissStats(uint16_t& misses, uint8_t& longestStreak);

  // Pojacanje prijemnika RC522, 0-7 (18 do 48 dB).
  void setGain(uint8_t level);
  uint8_t gain();

  // Brojaci od paljenja, za poredjenje podesavanja: pitanja dok je figurica
  // stajala, koliko ih je promaseno i koliko puta je proglasena skinutom.
  void counters(uint32_t& polls, uint32_t& misses, uint32_t& removals) const;

private:
  bool readUid(TagUid& out);

  bool _present = false;
  TagUid _uid;
  uint8_t _misses = 0;
  uint32_t _lastPoll = 0;
  uint16_t _missTotal = 0;
  uint8_t _longestStreak = 0;
  uint32_t _presentPolls = 0;
  uint32_t _presentMisses = 0;
  uint32_t _removals = 0;
};
