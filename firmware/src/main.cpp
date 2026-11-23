// Figurica radio - muzicka igracka sa NFC figuricama (ESP32-C3 SuperMini)
//
// Figurica na lezistu pusti svoju pesmu, skinuta je pauzira, a vracena u roku
// od RESUME_WINDOW_MS nastavlja gde je stala. Nepoznata figurica dobija prvu
// slobodnu pesmu i radio to pamti (vidi LearnedTags.h). Enkoder menja jacinu,
// klik na njega je pauza/nastavak, drzanje pusta pesmu ispocetka.
//
// Za probu na stolu postoje komande preko Serial monitora: play N, pause,
// resume, vol N, bat, info, list (koja figurica ima koju pesmu), forget N
// (oslobodi pesmu N), forget (oslobodi sve), log (dnevnik dogadjaja, vidi
// EventLog.h), gain N i rfid (pojacanje RC522), dfreset (restart DFPlayera).

#include <Arduino.h>
#include <Preferences.h>
#include <esp_system.h>

#include "Battery.h"
#include "Config.h"
#include "Encoder.h"
#include "EventLog.h"
#include "HoldButton.h"
#include "LearnedTags.h"
#include "Player.h"
#include "TagReader.h"

enum class State {
  Idle,          // nista ne svira
  Playing,
  Paused,        // pesma stoji i ceka: figurica skinuta ili klik na dugme
  Finished,      // pesma gotova, figurica jos stoji na lezistu
  BatteryEmpty,  // zastita baterije: ne svira dok se ne napuni
};

static State state = State::Idle;
static TagReader tags;
static LearnedTags learned;
static Player player;
static HoldButton button(PIN_ENC_SW, BUTTON_HOLD_MS, BUTTON_DOUBLE_CLICK_MS);
static Preferences prefs;

static uint16_t currentTrack = 0;  // pesma koja svira ili stoji u pauzi; 0 = nijedna
static uint32_t pausedAt = 0;

static uint8_t volume = VOLUME_DEFAULT;
static bool volumeDirty = false;
static uint32_t volumeChangedAt = 0;

static uint32_t lastBatteryCheck = 0;
static uint32_t lastLowWarning = 0;
static bool lowWarned = false;
static int criticalReads = 0;

// Koliko cesto se u dnevnik upise koliko je citanja taga promaseno dok je
// figurica stajala. Retko, da dnevnik ne prepune samo ti redovi.
constexpr uint32_t MISS_REPORT_MS = 10UL * 1000UL;
static uint32_t lastMissReport = 0;

static const char* stateName(State s) {
  switch (s) {
    case State::Idle: return "Idle";
    case State::Playing: return "Playing";
    case State::Paused: return "Paused";
    case State::Finished: return "Finished";
    case State::BatteryEmpty: return "BatteryEmpty";
  }
  return "?";
}

static void setState(State next) {
  if (next == state) return;
  logf("[state] %s -> %s", stateName(state), stateName(next));
  state = next;
}

static String uidText(const TagUid& uid) {
  String text;
  for (uint8_t i = 0; i < uid.len; i++) {
    char hex[4];
    snprintf(hex, sizeof(hex), "%s%02X", i ? ":" : "", uid.bytes[i]);
    text += hex;
  }
  return text;
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

static void startTrack(uint16_t track) {
  currentTrack = track;
  player.play(track);
  setState(State::Playing);
}

static void pauseTrack() {
  player.pause();
  pausedAt = millis();
  setState(State::Paused);
}

static void resumeTrack() {
  player.resume();
  setState(State::Playing);
}

// Sistemski zvuk prekida i zaboravlja pesmu: DFPlayer ima samo jedan tok, pa
// posle zvuka nema sta da se nastavi.
static void playSystemSound(uint16_t track) {
  if (track == 0) return;
  currentTrack = 0;
  player.play(track);
}

static void onTagPlaced() {
  const TagUid& uid = tags.uid();
  const String id = uidText(uid);
  uint16_t track = learned.find(uid);

  if (state == State::BatteryEmpty) {
    logf("[tag] figurica %s, ali baterija je prazna", id.c_str());
    playSystemSound(TRACK_LOW_BATTERY);
    return;
  }

  if (track != 0) {
    logf("[tag] figurica %s -> pesma %u", id.c_str(), track);
  } else {
    track = learned.assign(uid);
    if (track == 0) {
      logf("[tag] figurica %s je nova, a svih %u pesama je zauzeto", id.c_str(), SONG_COUNT);
      playSystemSound(TRACK_UNKNOWN);
      setState(State::Idle);
      return;
    }
    logf("[tag] figurica %s je nova, dobila je pesmu %u", id.c_str(), track);
  }

  const bool sameSong = state == State::Paused && track == currentTrack &&
                        millis() - pausedAt < RESUME_WINDOW_MS;
  if (sameSong) {
    resumeTrack();
  } else {
    startTrack(track);
  }
}

static void onTagRemoved() {
  logf("[tag] figurica skinuta (%u pitanja zaredom bez odgovora)", TAG_LOST_POLLS);
  if (state == State::Playing) {
    pauseTrack();
  } else if (state == State::Finished) {
    setState(State::Idle);
  }
}

static void onFinished() {
  // Kraj sistemskog zvuka ne menja stanje - samo kraj pesme koja svira.
  if (state != State::Playing) return;
  setState(tags.present() ? State::Finished : State::Idle);
}

static void onClick() {
  logf("[btn] klik");
  switch (state) {
    case State::Playing:
      pauseTrack();
      break;
    case State::Paused:
      // Bez figurice nema nastavka; dete je vraca na leziste.
      if (tags.present()) resumeTrack();
      break;
    case State::Finished:
      startTrack(currentTrack);
      break;
    default:
      break;
  }
}

static void onHold() {
  logf("[btn] drzanje");
  if (state == State::BatteryEmpty || currentTrack == 0 || !tags.present()) return;
  startTrack(currentTrack);
}

static void changeVolume(int delta) {
  const int next = constrain((int)volume + delta, 0, (int)VOLUME_MAX);
  if (next == volume) return;
  volume = (uint8_t)next;
  player.setVolume(volume);
  volumeDirty = true;
  volumeChangedAt = millis();
  logf("[vol] %u", volume);
}

static void saveVolumeWhenSettled() {
  if (!volumeDirty || millis() - volumeChangedAt < VOLUME_SAVE_DELAY_MS) return;
  prefs.putUChar("vol", volume);
  volumeDirty = false;
}

static void checkBattery() {
  const uint32_t mv = batteryRead().mv;

  // Baterija nije zakacena, ESP radi sa USB-a - nema sta da se stiti.
  if (mv < BATTERY_ABSENT_MV) {
    criticalReads = 0;
    return;
  }

  if (state == State::BatteryEmpty) {
    if (mv >= BATTERY_RESUME_MV) {
      logf("[batt] napunjena, radio ponovo svira");
      setState(State::Idle);
    }
    return;
  }

  if (mv < BATTERY_CRITICAL_MV) {
    if (++criticalReads < BATTERY_CRITICAL_READS) return;
    criticalReads = 0;
    logf("[batt] prazna, radio staje dok se ne napuni");
    if (state == State::Playing) player.pause();
    playSystemSound(TRACK_LOW_BATTERY);
    setState(State::BatteryEmpty);
    return;
  }
  criticalReads = 0;

  const uint32_t now = millis();
  const bool quiet = state == State::Idle || state == State::Finished;
  const bool due = !lowWarned || now - lastLowWarning >= BATTERY_WARN_EVERY_MS;
  if (mv < BATTERY_LOW_MV && quiet && due) {
    logf("[batt] slaba, upozorenje");
    lowWarned = true;
    lastLowWarning = now;
    playSystemSound(TRACK_LOW_BATTERY);
  }
}

static void handleSerialLine(String line) {
  line.trim();
  if (line.startsWith("play ")) {
    const int track = line.substring(5).toInt();
    if (track > 0) startTrack((uint16_t)track);
  } else if (line == "pause") {
    if (state == State::Playing) pauseTrack();
  } else if (line == "resume") {
    if (state == State::Paused) resumeTrack();
  } else if (line.startsWith("vol ")) {
    changeVolume(line.substring(4).toInt() - volume);
  } else if (line == "bat") {
    batteryRead();
  } else if (line == "info") {
    Serial.printf("[info] RC522 verzija 0x%02X, stanje %s, jacina %u, zauzeto %u od %u pesama\n",
                  tags.version(), stateName(state), volume, learned.count(), SONG_COUNT);
    Serial.printf("[info] fajlova na SD kartici: %d\n", player.fileCount());
    batteryRead();
  } else if (line == "list") {
    learned.print();
  } else if (line.startsWith("forget ")) {
    const int track = line.substring(7).toInt();
    if (learned.release((uint16_t)track)) {
      Serial.printf("[figurice] pesma %d je slobodna\n", track);
    } else {
      Serial.printf("[figurice] pesma %d nije bila zauzeta\n", track);
    }
  } else if (line == "forget") {
    learned.releaseAll();
    Serial.println("[figurice] sve pesme su slobodne");
  } else if (line == "log") {
    logDump();
  } else if (line == "dfreset") {
    player.resetModule();
  } else if (line.startsWith("gain ")) {
    tags.setGain((uint8_t)line.substring(5).toInt());
    logf("[rfid] pojacanje %u", tags.gain());
  } else if (line == "rfid") {
    uint32_t polls = 0, misses = 0, removals = 0;
    tags.counters(polls, misses, removals);
    Serial.printf("[rfid] pojacanje %u, pitanja dok stoji %lu, promaseno %lu, skidanja %lu\n",
                  tags.gain(), (unsigned long)polls, (unsigned long)misses, (unsigned long)removals);
  } else {
    Serial.println("[cmd] komande: play N | pause | resume | vol N | bat | info | list | forget N | forget | log | gain N | rfid | dfreset");
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
  Serial.begin(115200);
  // USB CDC: kratko se ceka monitor da uhvati pocetni log, ali bez racunara se
  // ne blokira - radio na bateriji mora odmah da krene.
  const uint32_t waitStart = millis();
  while (!Serial && millis() - waitStart < 1500) delay(10);
  Serial.println();
  logf("[boot] Figurica radio %s, start: %s", FW_VERSION, resetReasonText(esp_reset_reason()));

  prefs.begin("radio", false);
  volume = std::min<uint8_t>(prefs.getUChar("vol", VOLUME_DEFAULT), VOLUME_MAX);

  button.begin();
  encoderBegin();
  if (!tags.begin()) logf("[rfid] RC522 ne odgovara - proveri SPI zice i 3.3V");
  learned.begin();
  player.begin(volume);

  const int files = player.fileCount();
  if (files >= 0) {
    Serial.printf("[player] na SD kartici %d fajlova\n", files);
  } else {
    Serial.println("[player] SD kartica se ne vidi - mora FAT32, pesme u /mp3");
  }

  checkBattery();
  lastBatteryCheck = millis();
}

void loop() {
  switch (tags.poll()) {
    case TagReader::Event::Placed: onTagPlaced(); break;
    case TagReader::Event::Removed: onTagRemoved(); break;
    case TagReader::Event::None: break;
  }

  if (player.pollFinished()) onFinished();

  const int detents = encoderTakeDetents();
  if (detents != 0) changeVolume(detents);

  switch (button.poll()) {
    case HoldButton::Event::Click: onClick(); break;
    case HoldButton::Event::Hold: onHold(); break;
    default: break;
  }

  saveVolumeWhenSettled();

  if (millis() - lastMissReport >= MISS_REPORT_MS) {
    lastMissReport = millis();
    uint16_t misses = 0;
    uint8_t streak = 0;
    tags.takeMissStats(misses, streak);
    if (misses != 0) {
      logf("[tag] dok stoji: %u promasenih pitanja u 10 s, najduzi niz %u (pauza na %u)", misses,
           streak, TAG_LOST_POLLS);
    }
  }

  if (millis() - lastBatteryCheck >= BATTERY_CHECK_MS) {
    lastBatteryCheck = millis();
    checkBattery();
  }

  pollSerial();
  delay(2);
}
