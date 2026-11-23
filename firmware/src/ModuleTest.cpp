// Proba modula bez RFID citaca i bez SD kartice.
//
// Proverava sve sto moze bez muzike:
//   - enkoder: okretanje menja jacinu 0-30
//   - taster na enkoderu: klik je play/pauza, drzanje ponavlja proveru DFPlayera
//   - vezu sa DFPlayerom: posle okretanja se jacina procita nazad iz modula, pa
//     se vidi da rade obe zice, i ka modulu i od njega
//   - napon baterije preko delitelja
//
// LED na plocici blicne na svaki skljocaj i klik, pa se enkoder i taster vide
// i bez racunara. Na bateriji, gde monitora nema, LED javlja i ko se restartovao
// kad zvuk stane:
//   2 kratka bljeska  - ESP se upalio (ukljucenje ili je struja potpuno nestala)
//   5 brzih bljeskova - ESP je resetovan zbog pada napona (brownout)
//   1 dug, 2 sekunde  - DFPlayer se sam restartovao, ESP nije
// Ako zvuk stane a LED ne javi nista, niko se nije restartovao.
//
// Bez SD kartice DFPlayer nema sta da pusti i na play odgovori greskom. I to je
// dobar znak: komanda je stigla i modul odgovara. Sa karticom ista proba pusta
// prvu pesmu.
//
// Prebacuje se sa: pio run -e proba -t upload

#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>
#include <esp_system.h>

#include "Battery.h"
#include "Config.h"
#include "Encoder.h"
#include "HoldButton.h"

// LED na plocici, upaljena na LOW. GPIO8 je strapping pin, ali posle starta
// sme da bude izlaz; u mirovanju ostaje HIGH, isto sto cip trazi pri resetu.
constexpr int PIN_LED = 8;

constexpr int TEST_VOLUME_START = 15;
constexpr int TEST_VOLUME_MAX = 30;  // ceo opseg DFPlayera, ne decija granica

// Jacina se cita nazad iz modula tek kad okretanje stane - upit blokira do
// pola sekunde, pa bi usred okretanja gubio skljocaje.
constexpr uint32_t VOLUME_VERIFY_DELAY_MS = 500;
constexpr uint32_t BATTERY_PRINT_MS = 30UL * 1000UL;

static DFRobotDFPlayerMini df;
static HoldButton button(PIN_ENC_SW, BUTTON_HOLD_MS, BUTTON_DOUBLE_CLICK_MS);

static int volume = TEST_VOLUME_START;
static bool playing = false;
static bool verifyPending = false;
static uint32_t volumeChangedAt = 0;
static uint32_t ledOffAt = 0;
static uint32_t lastBatteryPrint = 0;
static bool started = false;  // setup() zavrsen; posle toga je javljanje DFPlayera restart

// Blokira, ali samo u setup()-u, gde nista drugo ne ceka.
static void flash(int times, uint32_t onMs, uint32_t offMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(PIN_LED, LOW);
    delay(onMs);
    digitalWrite(PIN_LED, HIGH);
    delay(offMs);
  }
}

static const char* resetReasonText(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "ukljucenje";
    case ESP_RST_BROWNOUT: return "pad napona (brownout)";
    case ESP_RST_SW: return "softverski";
    case ESP_RST_PANIC: return "pad programa";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT: return "watchdog";
    case ESP_RST_USB: return "USB";
    default: return "drugi razlog";
  }
}

static void blink(uint32_t ms) {
  digitalWrite(PIN_LED, LOW);
  ledOffAt = millis() + ms;
}

static void updateLed() {
  if (ledOffAt != 0 && (int32_t)(millis() - ledOffAt) >= 0) {
    digitalWrite(PIN_LED, HIGH);
    ledOffAt = 0;
  }
}

// Kodovi gresaka koje DFPlayer salje u poruci tipa DFPlayerError.
static const char* dfErrorText(uint16_t code) {
  switch (code) {
    case 1: return "modul zauzet ili nema kartice - bez SD kartice ocekivano";
    case 2: return "modul spava";
    case 3: return "pogresan okvir na RX - smetnje na liniji GPIO21 -> RX";
    case 4: return "pogresan checksum - smetnje na liniji GPIO21 -> RX";
    case 5: return "nema pesme sa tim brojem";
    case 6: return "fajl nije nadjen";
    case 7: return "reklama moze samo dok pesma svira";
    default: return "nepoznata greska";
  }
}

static void pollDfPlayer() {
  if (!df.available()) return;
  const uint8_t type = df.readType();
  const uint16_t value = df.read();

  switch (type) {
    case DFPlayerError:
      Serial.printf("[df] greska %u: %s\n", value, dfErrorText(value));
      playing = false;
      break;
    case DFPlayerCardOnline:
    case DFPlayerUSBOnline:
      // Ovo DFPlayer javlja kad se upali. Posle setup()-a to znaci da se sam
      // restartovao - najcesce zato sto mu je napon pao dok je pojacalo vuklo.
      if (started) {
        Serial.println("[df] DFPlayer se sam restartovao - verovatno je pao napon");
        blink(2000);
        playing = false;
        df.volume((uint8_t)volume);  // posle restarta se vraca na svoju jacinu
      } else {
        Serial.println("[df] uredjaj za muziku spreman");
      }
      break;
    case DFPlayerCardInserted:
      Serial.println("[df] SD kartica ubacena");
      break;
    case DFPlayerCardRemoved:
      Serial.println("[df] SD kartica izvadjena");
      break;
    case DFPlayerUSBInserted:
      Serial.println("[df] USB disk ubacen");
      break;
    case DFPlayerPlayFinished:
      Serial.printf("[df] pesma %u gotova\n", value);
      playing = false;
      break;
    default:
      Serial.printf("[df] poruka tipa %u, vrednost %u\n", type, value);
      break;
  }
}

// Upiti cekaju odgovor, a odgovor bi mogla da preduhitri poruka koja je vec
// stigla (npr. greska posle play). Zato se pre upita pokupi sve sto ceka.
static void drainDfPlayer() {
  for (int i = 0; i < 8 && df.available(); i++) {
    // available() je poruku vec procitao; ovde se ispisu samo greske.
    const uint8_t type = df.readType();
    const uint16_t value = df.read();
    if (type == DFPlayerError) Serial.printf("[df] greska %u: %s\n", value, dfErrorText(value));
  }
}

static int readVolumeFromModule() {
  int got = -1;
  for (int attempt = 0; attempt < 2 && got < 0; attempt++) {
    drainDfPlayer();
    got = df.readVolume();
  }
  return got;
}

static void checkDfPlayer() {
  Serial.println("[df] provera veze...");
  const int vol = readVolumeFromModule();
  if (vol < 0) {
    Serial.println("[df] NE ODGOVARA. Proveri: 5 V i GND na modulu, DFPlayer TX -> GPIO20,");
    Serial.println("[df] GPIO21 -> (1 kOhm) -> DFPlayer RX. TX i RX se ukrstaju.");
    return;
  }
  Serial.printf("[df] odgovara, jacina u modulu je %d - obe zice rade\n", vol);

  drainDfPlayer();
  const int files = df.readFileCounts();
  if (files < 0) {
    Serial.println("[df] SD kartica nije nadjena (bez kartice je to ocekivano)");
  } else {
    Serial.printf("[df] na SD kartici ima %d fajlova\n", files);
  }
}

static void verifyVolume() {
  if (!verifyPending || millis() - volumeChangedAt < VOLUME_VERIFY_DELAY_MS) return;
  verifyPending = false;

  const int got = readVolumeFromModule();
  if (got == volume) {
    Serial.printf("[df] potvrdio jacinu %d\n", got);
  } else if (got < 0) {
    Serial.println("[df] nije odgovorio na upit jacine - proveri obe zice ka DFPlayeru");
  } else {
    Serial.printf("[df] javlja jacinu %d umesto %d - komanda se izgubila na putu\n", got, volume);
  }
}

static void changeVolume(int delta) {
  const int next = constrain(volume + delta, 0, TEST_VOLUME_MAX);
  if (next == volume) {
    Serial.printf("[vol] %d (%s)\n", volume, volume == 0 ? "minimum" : "maksimum");
    return;
  }
  volume = next;
  df.volume((uint8_t)volume);
  Serial.printf("[vol] %d\n", volume);
  verifyPending = true;
  volumeChangedAt = millis();
}

static void togglePlay() {
  if (playing) {
    Serial.println("[btn] pauza");
    df.pause();
    playing = false;
  } else {
    Serial.println("[btn] play");
    df.start();
    playing = true;
  }
}

static void handleSerialLine(String line) {
  line.trim();
  if (line.startsWith("vol ")) {
    changeVolume(line.substring(4).toInt() - volume);
  } else if (line == "play") {
    if (!playing) togglePlay();
  } else if (line == "pause") {
    if (playing) togglePlay();
  } else if (line == "next") {
    Serial.println("[cmd] sledeca pesma");
    df.next();
    playing = true;
  } else if (line == "bat") {
    batteryRead();
  } else if (line == "info") {
    checkDfPlayer();
  } else {
    Serial.println("[cmd] komande: vol N | play | pause | next | bat | info");
  }
}

static void pollSerial() {
  static String line;
  while (Serial.available()) {
    const char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (line.length()) handleSerialLine(line);
      line = "";
    } else if (line.length() < 32) {
      line += c;
    }
  }
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);

  Serial.begin(115200);
  // Duze nego u pravom firmveru: ovde je ceo smisao da se vidi pocetni log.
  const uint32_t waitStart = millis();
  while (!Serial && millis() - waitStart < 3000) delay(10);
  const esp_reset_reason_t reason = esp_reset_reason();
  Serial.printf("\n[boot] Figurica radio - proba modula %s, start: %s\n", FW_VERSION,
                resetReasonText(reason));
  if (reason == ESP_RST_BROWNOUT) {
    flash(5, 80, 80);
  } else {
    flash(2, 150, 150);
  }

  button.begin();
  encoderBegin();

  Serial1.begin(9600, SERIAL_8N1, PIN_DF_RX, PIN_DF_TX);
  // Reset modula; begin() posle toga ceka do 2 s da se modul javi.
  df.begin(Serial1, false, true);
  df.volume((uint8_t)volume);
  checkDfPlayer();

  batteryRead();
  lastBatteryPrint = millis();

  Serial.println("[proba] okreni enkoder: jacina. Klik: play/pauza. Drzanje: provera DFPlayera.");
  Serial.println("[proba] komande: vol N | play | pause | next | bat | info");
  started = true;
}

void loop() {
  pollDfPlayer();

  const int detents = encoderTakeDetents();
  if (detents != 0) {
    changeVolume(detents);
    blink(40);
  }
  verifyVolume();

  switch (button.poll()) {
    case HoldButton::Event::Click:
      togglePlay();
      blink(200);
      break;
    case HoldButton::Event::DoubleClick:
      Serial.println("[btn] dvoklik - taster radi, ovde nema funkciju");
      blink(200);
      break;
    case HoldButton::Event::Hold:
      Serial.println("[btn] drzanje");
      blink(800);
      checkDfPlayer();
      batteryRead();
      break;
    case HoldButton::Event::None:
      break;
  }

  if (millis() - lastBatteryPrint >= BATTERY_PRINT_MS) {
    lastBatteryPrint = millis();
    batteryRead();
  }

  pollSerial();
  updateLed();
  delay(2);
}
