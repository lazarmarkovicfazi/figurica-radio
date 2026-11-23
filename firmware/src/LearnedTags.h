#pragma once
#include <Arduino.h>
#include <array>

#include "Config.h"
#include "TagReader.h"

// Koja figurica pusta koju pesmu. Pesme su numerisane 1..SONG_COUNT (fajlovi
// /mp3/0001.mp3 ...), a svaka figurica zauzima jedan broj. Nepoznata figurica
// dobija prvi slobodan broj; veza se pamti u flash-u i vazi i posle gasenja.
class LearnedTags {
public:
  // Ucitava zauzete brojeve iz flash-a u RAM.
  void begin();

  // Broj pesme za figuricu, 0 ako je figurica nepoznata.
  uint16_t find(const TagUid& uid) const;

  // Daje figurici prvi slobodan broj i pamti ga; 0 ako su svi zauzeti.
  uint16_t assign(const TagUid& uid);

  // Oslobadja broj, pa ga dobija sledeca nepoznata figurica. false ako je
  // broj vec bio slobodan.
  bool release(uint16_t track);
  void releaseAll();

  uint16_t count() const;
  void print() const;

private:
  // Indeks + 1 je broj pesme; prazan UID (len 0) znaci da je broj slobodan.
  std::array<TagUid, SONG_COUNT> _slots;
};
