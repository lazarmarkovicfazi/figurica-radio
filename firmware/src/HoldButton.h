#pragma once
#include <Arduino.h>  // pinMode, digitalRead, millis

// Dugme (aktivno LOW) sa tri ishoda: klik, dvoklik i drzanje. Nonblocking.
//
// Jedna metoda, ne vise njih: sve bi morale da citaju pin i menjaju stanje, pa
// bi ona koja se pozove druga videla vec potrosen pritisak.
//
// Drzanje okine cim se predje prag, jos dok je dugme dole - da korisnik dobije
// odziv i zna da moze da pusti. Klik naprotiv ceka otpustanje: dok je dugme
// dole ne zna se da li ce ostati kratak pritisak ili prerasti u drzanje.
//
// Klik uz to ceka jos `doubleMs` posle otpustanja, jer se tek tada zna da za
// njim ne stize drugi. To je cena dvoklika: obican klik kasni za taj prozor.
class HoldButton {
public:
  enum class Event {
    None,
    Click,        // kratak pritisak, prijavljen kad istekne prozor za dvoklik
    DoubleClick,  // dva kratka pritiska, prijavljen pri otpustanju drugog
    Hold,         // drzanje preslo prag, prijavljeno odmah
  };

  HoldButton(int pin, uint32_t holdMs, uint32_t doubleMs)
      : _pin(pin), _holdMs(holdMs), _doubleMs(doubleMs) {}

  void begin() { pinMode(_pin, INPUT_PULLUP); }

  // Pritisak koji je upravo u toku proglasava potrosenim: nece dati ni klik ni
  // drzanje. Za drzanje koje je vec prepoznato van poll()-a, pri startu - bez
  // ovoga bi se po otpustanju prijavio jos i klik.
  void swallowPress() {
    if (digitalRead(_pin) != LOW) return;
    _pressedSince = millis();
    _fired = true;
  }

  Event poll() {
    const bool pressed = (digitalRead(_pin) == LOW);
    const uint32_t now = millis();

    if (pressed) {
      if (_pressedSince == 0) {
        _pressedSince = now;
        return Event::None;
      }
      if (!_fired && (now - _pressedSince) >= _holdMs) {
        _fired = true;
        // Klik pa drzanje je drzanje - klik koji je cekao drugi se odbacuje.
        _clickPending = false;
        return Event::Hold;
      }
      return Event::None;
    }

    const uint32_t pressedAt = _pressedSince;
    const uint32_t held = pressedAt == 0 ? 0 : now - pressedAt;
    const bool wasHold = _fired;
    _pressedSince = 0;
    _fired = false;

    // Kraci pritisci od DEBOUNCE_MS su treperenje kontakta, ne namera.
    // Otpustanje posle drzanja nije klik - taj pritisak je vec potrosen.
    if (!wasHold && held >= DEBOUNCE_MS && held < _holdMs) {
      // Prozor se meri od otpustanja prvog do pocetka drugog pritiska, ne do
      // njegovog kraja: koliko dugo korisnik drzi drugi pritisak ne sme da
      // odlucuje da li je to bio dvoklik.
      if (_clickPending && (pressedAt - _clickAt) <= _doubleMs) {
        _clickPending = false;
        return Event::DoubleClick;
      }

      // Prvi klik - ceka da se vidi stize li drugi. Ako je neki vec cekao, a
      // ovaj je stigao prekasno (poll() je stajao dok je trajalo javljanje),
      // onaj stari se prijavljuje sad, a ovaj ceka svoj red.
      const bool stale = _clickPending;
      _clickPending = true;
      _clickAt = now;
      return stale ? Event::Click : Event::None;
    }

    if (_clickPending && (now - _clickAt) > _doubleMs) {
      _clickPending = false;
      return Event::Click;
    }
    return Event::None;
  }

private:
  static constexpr uint32_t DEBOUNCE_MS = 40;

  const int _pin;
  const uint32_t _holdMs;
  const uint32_t _doubleMs;
  uint32_t _pressedSince = 0;
  bool _fired = false;
  bool _clickPending = false;  // klik je otpusten i ceka da istekne prozor
  uint32_t _clickAt = 0;       // kad je otpusten
};
