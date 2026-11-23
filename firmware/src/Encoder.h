#pragma once

// Kvadraturni enkoder na prekidima. Koraci se broje u prekidu, a petlja ih
// pokupi kao cele skljocaje: RC522 ume da drzi petlju desetinama milisekundi,
// pa bi citanje pinova iz petlje gubilo korake pri brzom okretanju.
void encoderBegin();

// Skljocaji od proslog poziva; pozitivno je u smeru kazaljke (vidi ENC_REVERSE).
int encoderTakeDetents();
