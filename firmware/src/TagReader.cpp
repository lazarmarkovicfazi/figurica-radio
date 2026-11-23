#include "TagReader.h"

#include <MFRC522.h>
#include <SPI.h>

#include "Config.h"

namespace {
MFRC522 rfid(PIN_RC522_SS, MFRC522::UNUSED_PIN);
}

bool TagReader::begin() {
  SPI.begin(PIN_RC522_SCK, PIN_RC522_MISO, PIN_RC522_MOSI, PIN_RC522_SS);
  rfid.PCD_Init();

  // Ne puno pojacanje: vidi RFID_RX_GAIN u Config.h.
  setGain(RFID_RX_GAIN);

  const uint8_t v = version();
  Serial.printf("[rfid] RC522 verzija 0x%02X\n", v);
  return v != 0x00 && v != 0xFF;
}

uint8_t TagReader::version() {
  return rfid.PCD_ReadRegister(MFRC522::VersionReg);
}

TagReader::Event TagReader::poll() {
  const uint32_t now = millis();
  if (now - _lastPoll < TAG_POLL_MS) return Event::None;
  _lastPoll = now;

  // Jedno promaseno citanje nije dokaz da figurice nema: na bateriji ga cesto
  // pokvare smetnje od step-upa i pojacala, bas dok pesma svira. Odmah se
  // pokusa jos jednom, pre nego sto se racuna promasaj.
  if (_present) _presentPolls++;
  TagUid seen;
  if (readUid(seen) || readUid(seen)) {
    _misses = 0;
    rfid.PICC_HaltA();
    if (_present && seen == _uid) return Event::None;
    // Nova figurica, ili zamenjena drugom brze nego sto bi se priznalo skidanje.
    _uid = seen;
    _present = true;
    return Event::Placed;
  }

  if (_present) {
    _misses++;
    _missTotal++;
    _presentMisses++;
    if (_misses > _longestStreak) _longestStreak = _misses;
    if (_misses >= TAG_LOST_POLLS) {
      _present = false;
      _misses = 0;
      _removals++;
      return Event::Removed;
    }
  }
  return Event::None;
}

void TagReader::setGain(uint8_t level) {
  rfid.PCD_SetAntennaGain((byte)(min<uint8_t>(level, 7) << 4));
}

uint8_t TagReader::gain() {
  return rfid.PCD_GetAntennaGain() >> 4;
}

void TagReader::counters(uint32_t& polls, uint32_t& misses, uint32_t& removals) const {
  polls = _presentPolls;
  misses = _presentMisses;
  removals = _removals;
}

void TagReader::takeMissStats(uint16_t& misses, uint8_t& longestStreak) {
  misses = _missTotal;
  longestStreak = _longestStreak;
  _missTotal = 0;
  _longestStreak = 0;
}

bool TagReader::readUid(TagUid& out) {
  // WUPA, ne REQA: budi i tag koji je posle proslog citanja poslat u HALT. Sa
  // REQA (PICC_IsNewCardPresent) bi se figurica koja stoji videla samo prvi
  // put, pa bi posle toga izgledalo kao da je skinuta.
  byte atqa[2];
  byte atqaSize = sizeof(atqa);
  const MFRC522::StatusCode status = rfid.PICC_WakeupA(atqa, &atqaSize);
  if (status != MFRC522::STATUS_OK && status != MFRC522::STATUS_COLLISION) return false;
  if (!rfid.PICC_ReadCardSerial()) return false;

  out.len = min((byte)sizeof(out.bytes), rfid.uid.size);
  memcpy(out.bytes, rfid.uid.uidByte, out.len);
  return true;
}
