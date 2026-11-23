#pragma once
#include <Arduino.h>

// Dnevnik dogadjaja u RAM-u. Svaki red ide i na Serial, a poslednjih nekoliko
// desetina ostaje u memoriji sa vremenom od paljenja. Tako se vidi sta je radio
// radio na bateriji, gde monitora nema: posle problema se, bez gasenja, ukljuci
// USB i posalje komanda `log`. Gasenje brise dnevnik.
void logf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void logDump();
