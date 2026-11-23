#pragma once
#include <Arduino.h>  // uint8_t, uint16_t, uint32_t

#define FW_VERSION "0.1.0"

// Pinovi ESP32-C3 SuperMini, isti kao na semi povezivanja.
//
// GPIO2, GPIO8 i GPIO9 su strapping pinovi: cip ih cita u trenutku reseta (8
// nosi i LED na plocici, 9 je BOOT taster). Na njima nema nicega sto bi moglo
// da ih povuce na pogresan nivo, pa plocica uvek normalno startuje - i kad
// dete drzi dugme dok pali radio.

// RC522 na podrazumevanim SPI pinovima Arduino jezgra za C3. RST je na samom
// modulu premoscen na 3.3V, pa biblioteka radi softverski reset.
constexpr int PIN_RC522_SCK = 4;
constexpr int PIN_RC522_MISO = 5;
constexpr int PIN_RC522_MOSI = 6;
constexpr int PIN_RC522_SS = 7;

// Gol enkoder (EC11), bez modula: srednji pin trojke i jedan pin tastera idu
// na masu, a sva tri ulaza drze interni pull-up otpornici. CLK je A, DT je B.
constexpr int PIN_ENC_CLK = 0;
constexpr int PIN_ENC_DT = 1;
constexpr int PIN_ENC_SW = 10;

// DFPlayer na UART1. Po GPIO21 ROM bootloader pri svakom paljenju ispise svoj
// log na 115200; DFPlayer slusa na 9600 i to vidi kao smece bez ispravnog
// okvira, pa ga ignorise.
constexpr int PIN_DF_RX = 20;  // prima sa DFPlayer TX
constexpr int PIN_DF_TX = 21;  // preko 1 kOhm na DFPlayer RX

// Polovina napona baterije: delitelj 2 x 100 kOhm, posle prekidaca, pa ne
// prazni celiju dok je radio ugasen.
constexpr int PIN_BATTERY = 3;

// Jacina zvuka, DFPlayer skala 0-30. Gornja granica stiti decije usi, a i
// bateriju: na punoj jacini pojacalo vuce struju koja obara napon celije.
constexpr uint8_t VOLUME_DEFAULT = 12;
constexpr uint8_t VOLUME_MAX = 22;

// Jacina se pamti u flash-u, ali tek kad se okretanje smiri - upis pri svakom
// skljocaju bi trosio flash bez potrebe.
constexpr uint32_t VOLUME_SAVE_DELAY_MS = 3000;

// EC11 sa 20 skljocaja po krugu prodje ceo ciklus (4 promene) izmedju dva
// skljocaja. Verzija sa 30 skljocaja prodje pola ciklusa - ako se jacina menja
// tek na svaki drugi skljocaj, ovde ide 2. Ako jacina raste na pogresnu
// stranu, ENC_REVERSE ide na true.
constexpr int ENC_STEPS_PER_DETENT = 4;
constexpr bool ENC_REVERSE = false;

// Dugme na enkoderu: klik je pauza/nastavak, drzanje pusta pesmu ispocetka.
// Dvoklik se ne koristi; prozor je kratak jer za toliko kasni svaki klik.
constexpr uint32_t BUTTON_HOLD_MS = 1500;
constexpr uint32_t BUTTON_DOUBLE_CLICK_MS = 250;

// Koliko cesto se pita RC522 da li je figurica tu. Jedno pitanje bez figurice
// traje do tridesetak milisekundi, pa cesce nema smisla.
constexpr uint32_t TAG_POLL_MS = 120;

// Pojedinacno citanje ume da promasi i dok figurica stoji, pa se skidanje
// priznaje tek posle ovoliko uzastopnih promasaja - oko jedne sekunde. Sa
// pola sekunde je pesma na bateriji secala: smetnje dok svira obore citanje,
// pauza ih ugasi, figurica se opet vidi, pesma nastavi - i tako u krug.
constexpr uint8_t TAG_LOST_POLLS = 8;

// Pojacanje prijemnika RC522, 0-7: 4 = 33 dB (fabricka vrednost cipa),
// 5 = 38, 6 = 43, 7 = 48 dB. Izmereno na zatvorenom poklopcu dok pesma svira,
// 25 s po nivou: na 48 dB promaseno 84% pitanja i 10 pauza, na 33-43 dB
// 0-1% i nijedna pauza. Puno pojacanje pojaca i smetnje, pa citanje pada.
constexpr uint8_t RFID_RX_GAIN = 4;

// Figurica vracena u ovom roku nastavlja pesmu gde je stala; posle toga ide
// ispocetka, jer se posle duze pauze niko ne seca gde je stala.
constexpr uint32_t RESUME_WINDOW_MS = 10UL * 60UL * 1000UL;

// DFPlayer ume da javi kraj prethodne pesme tek posto je nova krenula. Javljanje
// koje stigne ovoliko brzo posle pustanja se zato ne racuna.
constexpr uint32_t FINISH_IGNORE_MS = 600;

// Pesme na kartici: /mp3/0001.mp3 do /mp3/0100.mp3. Svaka figurica zauzima
// jedan broj. Nepoznata figurica dobija prvi slobodan broj, i radio to pamti i
// posle gasenja. `forget N` u Serial monitoru oslobadja broj N, a `forget`
// sve brojeve.
constexpr uint16_t SONG_COUNT = 100;

// Sistemski zvuci u /mp3 na SD kartici; 0 znaci bez zvuka. Ako fajla nema,
// DFPlayer samo prijavi gresku i radio cuti.
constexpr uint16_t TRACK_UNKNOWN = 998;      // nova figurica, a sve pesme su zauzete
constexpr uint16_t TRACK_LOW_BATTERY = 999;  // vreme je za punjac

// Merenje baterije. Delitelj je 1:2, pa se napon na pinu mnozi sa dva.
constexpr float BATTERY_DIVIDER = 2.0f;
constexpr int BATTERY_SAMPLES = 16;
constexpr uint32_t BATTERY_CHECK_MS = 30UL * 1000UL;

// Upozorenje se pusta samo kad nista ne svira, da ne prekine pesmu, i ne
// cesce od BATTERY_WARN_EVERY_MS.
constexpr uint32_t BATTERY_LOW_MV = 3450;
constexpr uint32_t BATTERY_WARN_EVERY_MS = 5UL * 60UL * 1000UL;

// Ispod ovoga radio prestaje da svira dok se ne napuni. Zastita na TP4056 bi
// celiju pustila do 2,4 V, sto joj skracuje vek.
//
// Glasan deo pesme na trenutak obori napon, pa se prag priznaje tek posle
// BATTERY_CRITICAL_READS uzastopnih merenja ispod njega.
constexpr uint32_t BATTERY_CRITICAL_MV = 3250;
constexpr int BATTERY_CRITICAL_READS = 3;

// Izlazak iz zastite. Pod punjenjem je napon odmah iznad ovoga, a sam oporavak
// celije u mirovanju ga ne prelazi.
constexpr uint32_t BATTERY_RESUME_MV = 3550;

// Ispod ovoga baterija nije zakacena: ESP radi sa USB-a, prekidac je ugasen
// pa je delitelj bez napona. Tada se zastita ne ukljucuje, da radio na stolu
// moze da se proba.
constexpr uint32_t BATTERY_ABSENT_MV = 2500;
