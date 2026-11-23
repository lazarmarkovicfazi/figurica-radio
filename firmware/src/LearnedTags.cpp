#include "LearnedTags.h"

#include <Preferences.h>

namespace {

Preferences store;

// Jedan zapis po zauzetom broju: kljuc "u<broj pesme>", vrednost su bajtovi
// UID-a. Isti oblik je pisala i prethodna verzija, koja je brojeve delila
// redom, pa vec naucene figurice ostaju na svojim brojevima.
String keyFor(uint16_t track) {
  return String("u") + track;
}

}  // namespace

void LearnedTags::begin() {
  store.begin("figurice", false);

  // Brojac iz prethodne verzije; sad se slobodan broj trazi, ne broji.
  if (store.isKey("n")) store.remove("n");

  for (uint16_t track = 1; track <= SONG_COUNT; track++) {
    TagUid& slot = _slots[track - 1];
    slot = TagUid();
    const String key = keyFor(track);
    if (store.isKey(key.c_str())) {
      slot.len = (uint8_t)store.getBytes(key.c_str(), slot.bytes, sizeof(slot.bytes));
    }
  }
  Serial.printf("[figurice] zauzeto %u od %u pesama\n", count(), SONG_COUNT);
}

uint16_t LearnedTags::find(const TagUid& uid) const {
  for (uint16_t i = 0; i < SONG_COUNT; i++) {
    if (_slots[i].len != 0 && _slots[i] == uid) return i + 1;
  }
  return 0;
}

uint16_t LearnedTags::assign(const TagUid& uid) {
  for (uint16_t i = 0; i < SONG_COUNT; i++) {
    if (_slots[i].len != 0) continue;
    _slots[i] = uid;
    store.putBytes(keyFor(i + 1).c_str(), uid.bytes, uid.len);
    return i + 1;
  }
  return 0;
}

bool LearnedTags::release(uint16_t track) {
  if (track == 0 || track > SONG_COUNT || _slots[track - 1].len == 0) return false;
  _slots[track - 1] = TagUid();
  store.remove(keyFor(track).c_str());
  return true;
}

void LearnedTags::releaseAll() {
  store.clear();
  _slots.fill(TagUid());
}

uint16_t LearnedTags::count() const {
  uint16_t n = 0;
  for (const TagUid& slot : _slots) {
    if (slot.len != 0) n++;
  }
  return n;
}

void LearnedTags::print() const {
  if (count() == 0) {
    Serial.println("[figurice] nijedna figurica jos nema pesmu");
    return;
  }
  for (uint16_t i = 0; i < SONG_COUNT; i++) {
    if (_slots[i].len == 0) continue;
    Serial.printf("[figurice] pesma %u: {", i + 1);
    for (uint8_t b = 0; b < _slots[i].len; b++) {
      Serial.printf("%s0x%02X", b ? ", " : "", _slots[i].bytes[b]);
    }
    Serial.println("}");
  }
}
