# 4. Učitavanje koda na pločicu

[← Lemljenje](3-lemljenje.md) · [Sledeći korak: pesme →](5-pesme.md)

Kod je u folderu [`firmware`](../firmware). Na pločicu se učitava iz programa **VS Code** sa dodatkom **PlatformIO**, koji sam skine sve što treba: kompajler, biblioteke i alat za upload.

## Instalacija, jednom

1. Instaliraj [VS Code](https://code.visualstudio.com/).
2. U VS Code-u otvori *Extensions* (Ctrl+Shift+X), potraži **PlatformIO IDE** i klikni *Install*. Kad završi, restartuj VS Code.
3. Skini ovaj projekat: na vrhu GitHub strane zeleno dugme *Code → Download ZIP*, pa raspakuj ZIP.
4. U VS Code-u: *File → Open Folder* i izaberi folder **`firmware`**, onaj u kome je `platformio.ini`.

## Prvi build

U donjoj plavoj traci VS Code-a su PlatformIO dugmići:

| Dugme | Šta radi |
|---|---|
| ✓ (Build) | prevodi kod, ništa ne šalje na pločicu |
| → (Upload) | prevodi i šalje na pločicu |
| utikač (Serial Monitor) | prikazuje šta pločica piše i prima komande |
| `env:...` | bira šta se učitava: `esp32c3` je radio, `proba` je proba modula |

Klikni ✓. Prvi put PlatformIO skida platformu za ESP32 (nekoliko stotina MB) i prevodi celo Arduino jezgro. To traje od 5 minuta do pola sata, a na Windowsu sa antivirusom i duže. Sledeći build traje desetak sekundi. Na kraju mora da piše **SUCCESS**.

## Upload

1. Isključi prekidač radija.
2. Poveži ESP32-C3 sa računarom USB-C kablom za podatke.
3. U donjoj traci izaberi `env:esp32c3` i klikni → (Upload).

Ako upload javi *Failed to connect* ili ne nađe port, prebaci pločicu ručno u režim za upload:

1. Drži taster **BOOT** na pločici.
2. Kratko pritisni taster **RST** (ili isključi pa uključi USB kabl).
3. Pusti BOOT i ponovo klikni Upload.
4. Kad upload završi, pritisni RST.

Ovo obično treba samo prvi put. Kad je kod jednom na pločici, sledeći uploadi idu sami.

Ako upload javi da je port zauzet, zatvori Serial Monitor pa pokušaj ponovo.

## Serial Monitor

Klikni na utikač. Posle pritiska na RST pločica ispiše šta je zatekla. Primer:

```
[boot] Figurica radio 0.1.0, start: ukljucenje
[rfid] RC522 verzija 0x92
[figurice] zauzeto 0 od 100 pesama
[player] DFPlayer na GPIO20/21, jacina 12
[player] na SD kartici 12 fajlova
```

- `RC522 verzija` treba da bude 0x91 ili 0x92 za originalni čip, a 0x82, 0x88 ili 0xB2 za kopije, koje takođe rade. 0x00 ili 0xFF znače da RC522 ne odgovara: proveri žice.
- `SD kartica se ne vidi` znači da kartica nije FAT32 ili nema folder `mp3` (vidi [korak 5](5-pesme.md)).

U Serial Monitoru se kucaju komande i šalju sa Enter:

| Komanda | Šta radi |
|---|---|
| `info` | stanje radija, verzija RC522, broj fajlova na kartici, napon baterije |
| `list` | koja figurica ima koju pesmu |
| `forget 5` | oslobađa pesmu 5; figurica koja ju je imala dobiće novu kad je sledeći put spustiš |
| `forget` | briše sve figurice |
| `play 5`, `pause`, `resume` | pusti pesmu 5, pauza, nastavak |
| `vol 15` | jačina 15 (0–30, ali najviše koliko je `VOLUME_MAX`) |
| `bat` | izmeri bateriju |
| `log` | poslednjih 80 događaja (vidi [korak 8](8-problemi.md#dnevnik-događaja)) |
| `rfid` | koliko je puta čitač promašio figuricu koja stoji |
| `gain 5` | privremeno menja pojačanje čitača, 0–7 |
| `dfreset` | restartuje DFPlayer, za probu |

## Proba modula pre sklapanja

Okruženje `proba` proverava sve osim čitača, i bez SD kartice:

- Okretanje enkodera menja jačinu, a svaki škljocaj blicne LED na pločici.
- Klik na dugme šalje DFPlayeru *play/pauza*. Bez kartice DFPlayer odgovori greškom, i to je dobar znak: veza radi u oba smera.
- Posle okretanja kod pročita jačinu nazad iz DFPlayera. Tako se vidi da rade i žica ka DFPlayeru i žica od njega.
- Napon baterije se ispisuje na 30 sekundi.

U donjoj traci izaberi `env:proba`, klikni Upload i otvori Serial Monitor. Kad proveriš module, vrati `env:esp32c3` i ponovo klikni Upload.

## Podešavanja

Sva podešavanja su u [`firmware/src/Config.h`](../firmware/src/Config.h). Posle izmene ponovo klikni Upload. Memorija figurica i jačina ostaju sačuvane.

| Podešavanje | Vrednost | Šta je |
|---|---|---|
| `VOLUME_DEFAULT` | 12 | jačina pri prvom paljenju, posle se pamti poslednja |
| `VOLUME_MAX` | 22 | najveća jačina (DFPlayer ide do 30); čuva dečije uši i bateriju |
| `ENC_STEPS_PER_DETENT` | 4 | stavi 2 ako se jačina menja tek na svaki drugi škljocaj |
| `ENC_REVERSE` | false | stavi true ako jačina raste na pogrešnu stranu |
| `BUTTON_HOLD_MS` | 1500 | koliko se drži dugme da pesma krene ispočetka |
| `RESUME_WINDOW_MS` | 10 min | figurica vraćena u ovom roku nastavlja pesmu, posle ide ispočetka |
| `RFID_RX_GAIN` | 4 | pojačanje čitača, vidi [korak 8](8-problemi.md#pesma-seče-ili-se-figurica-ne-vidi-kroz-poklopac) |
| `SONG_COUNT` | 100 | koliko pesama, tj. figurica, radio ume da zapamti |

## Iz komandne linije

Ako ti je lakše bez VS Code-a, sa instaliranim [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html), iz foldera `firmware`:

```bash
pio run -e esp32c3 -t upload
```

```bash
pio device monitor
```

[Sledeći korak: pesme na SD kartici →](5-pesme.md)
