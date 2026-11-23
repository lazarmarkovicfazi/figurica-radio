#include "Encoder.h"

#include <Arduino.h>
#include <soc/gpio_reg.h>
#include <soc/soc.h>

#include "Config.h"

namespace {

// Prekid moze da stigne bas dok se jacina upisuje u flash, a tada flash nije
// dostupan. Zato je sve sto prekid dira u RAM-u: tabela sa DRAM_ATTR, pinovi
// se citaju direktno iz registra umesto kroz digitalRead(), koji je u flash-u.
volatile int32_t steps = 0;
volatile uint8_t lastAB = 0;
int32_t consumed = 0;

// Prelaz izmedju dva uzastopna stanja AB -> korak -1, 0 ili +1. Nemoguci
// prelazi (preskocen korak, treperenje kontakta) daju 0.
DRAM_ATTR const int8_t TRANSITION[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

void IRAM_ATTR onEdge() {
  const uint32_t in = REG_READ(GPIO_IN_REG);
  const uint8_t ab = (uint8_t)((((in >> PIN_ENC_CLK) & 1) << 1) | ((in >> PIN_ENC_DT) & 1));
  steps += TRANSITION[(lastAB << 2) | ab];
  lastAB = ab;
}

}  // namespace

void encoderBegin() {
  // Gol enkoder samo spaja pin sa masom; bez pull-up-a bi ulaz plutao.
  pinMode(PIN_ENC_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_DT, INPUT_PULLUP);
  lastAB = (uint8_t)((digitalRead(PIN_ENC_CLK) << 1) | digitalRead(PIN_ENC_DT));
  attachInterrupt(PIN_ENC_CLK, onEdge, CHANGE);
  attachInterrupt(PIN_ENC_DT, onEdge, CHANGE);
}

int encoderTakeDetents() {
  // Samo prekid pise u `steps`, a 32-bitno citanje na C3 je atomicno. Odavde
  // se uzimaju samo celi skljocaji; ostatak ceka sledeci poziv.
  const int32_t raw = steps;
  const int32_t detents = (raw - consumed) / ENC_STEPS_PER_DETENT;
  consumed += detents * ENC_STEPS_PER_DETENT;
  return ENC_REVERSE ? -detents : detents;
}
