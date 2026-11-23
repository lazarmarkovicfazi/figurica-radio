#include "Player.h"

#include <DFRobotDFPlayerMini.h>

#include "Config.h"
#include "EventLog.h"

namespace {
DFRobotDFPlayerMini df;
}

void Player::begin(uint8_t volume) {
  Serial1.begin(9600, SERIAL_8N1, PIN_DF_RX, PIN_DF_TX);

  // Bez potvrda (isACK = false): kopije DFPlayer cipa ih salju nepouzdano, pa
  // bi begin() odbio ispravan modul. Cena je sto se odsutan modul ne vidi
  // ovde, nego tek kao tisina - zato se ispise sta se ocekuje.
  df.begin(Serial1, false, true);
  _volume = volume;
  df.volume(volume);
  logf("[player] DFPlayer na GPIO%d/%d, jacina %u", PIN_DF_RX, PIN_DF_TX, volume);
}

void Player::play(uint16_t track) {
  logf("[player] pustam /mp3/%04u.mp3", track);
  df.playMp3Folder(track);
  _track = track;
  _resumable = true;
  _startedAt = millis();
}

void Player::pause() {
  logf("[player] pauza");
  df.pause();
}

void Player::resume() {
  if (!_resumable) {
    logf("[player] pesma %u se ne moze nastaviti (DFPlayer restartovan ili gotova), ide ispocetka",
         _track);
    play(_track);
    return;
  }
  logf("[player] nastavak pesme %u", _track);
  df.start();
  _startedAt = millis();
}

void Player::setVolume(uint8_t volume) {
  _volume = volume;
  df.volume(volume);
}

void Player::resetModule() {
  logf("[player] reset DFPlayera");
  df.reset();
}

int Player::fileCount() {
  // Odgovor na upit bi mogla da preduhitri poruka koja vec ceka (npr. ona
  // koju modul posalje posle reseta), pa se pre upita sve pokupi.
  for (int i = 0; i < 8 && df.available(); i++) {
    df.readType();
    df.read();
  }
  return df.readFileCounts();
}

bool Player::pollFinished() {
  if (!df.available()) return false;
  const uint8_t type = df.readType();
  const uint16_t value = df.read();

  switch (type) {
    case DFPlayerPlayFinished:
      // DFPlayer kraj ume da javi dvaput, a i zakasnelo, posle nove pesme.
      if (millis() - _startedAt < FINISH_IGNORE_MS) return false;
      _startedAt = millis();
      _resumable = false;
      logf("[player] pesma %u gotova", _track);
      return true;
    case DFPlayerError:
      // 6 je "fajl ne postoji" - najcesce nema pesme sa tim brojem u /mp3.
      _resumable = false;
      logf("[player] greska %u%s", value, value == 6 ? " (nema fajla)" : "");
      return false;
    case DFPlayerCardRemoved:
      logf("[player] SD kartica izvadjena");
      return false;
    case DFPlayerCardInserted:
      logf("[player] SD kartica ubacena");
      return false;
    case DFPlayerCardOnline:
      // Ovo modul javlja kad se upali; usred rada znaci da se sam restartovao,
      // najcesce zbog pada napona na glasnom mestu u pesmi. Posle restarta ne
      // pamti ni pesmu ni jacinu (vraca svoju fabricku, glasniju).
      logf("[player] DFPlayer se restartovao, vracam jacinu %u", _volume);
      df.volume(_volume);
      _resumable = false;
      return false;
    default:
      return false;
  }
}
