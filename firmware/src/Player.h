#pragma once
#include <Arduino.h>

// DFPlayer Mini preko UART1. Tanak omotac oko DFRobot biblioteke: drzi port i
// filtrira javljanja, pa main.cpp vidi samo "pesma je gotova".
class Player {
public:
  void begin(uint8_t volume);

  // Pesma /mp3/NNNN.mp3 sa SD kartice.
  void play(uint16_t track);
  void pause();

  // Nastavlja pauziranu pesmu. Ako se DFPlayer u medjuvremenu restartovao ili
  // je pesma vec dosla do kraja, pusta istu pesmu ispocetka: komanda za
  // nastavak (0x0D) bi tada pustila prvi fajl na kartici, tj. tudju pesmu.
  void resume();
  void setVolume(uint8_t volume);

  // Softverski reset DFPlayera, za probu oporavka (komanda "dfreset").
  void resetModule();

  // true jednom, kad DFPlayer javi da je pesma koja svira dosla do kraja.
  bool pollFinished();

  // Broj fajlova na SD kartici, -1 ako se kartica ne vidi ili modul cuti.
  // Ceka odgovor do pola sekunde, pa je za start i dijagnostiku, ne za petlju.
  int fileCount();

private:
  uint32_t _startedAt = 0;
  uint16_t _track = 0;      // poslednja pustena pesma
  uint8_t _volume = 0;      // vraca se posle restarta modula
  bool _resumable = false;  // da li DFPlayer jos drzi _track u pauzi
};
