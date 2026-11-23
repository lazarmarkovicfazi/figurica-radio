# 8. Kad nešto ne radi

[← Korišćenje](7-koriscenje.md) · [Nazad na početak](../README.md)

Skoro svaki problem se najbrže nađe u Serial Monitoru ([korak 4](4-kod.md#serial-monitor)): radio ispisuje svaki događaj, pa se vidi šta se desilo i kojim redom.

## Dnevnik događaja

Radio pamti poslednjih 80 događaja: figurica spuštena ili skinuta, pesma, pauza, jačina, restart DFPlayera, baterija. Kad se problem desi dok radi na bateriji:

1. **Ne gasi radio.** Gašenje briše dnevnik.
2. Poveži ESP na računar i otvori Serial Monitor.
3. Pošalji `log`.

Svaki red ima vreme od paljenja, pa se vidi i šta je bilo neposredno pre problema.

## Računar ne vidi pločicu ili upload ne uspeva

- Probaj drugi USB kabl. Mnogi kablovi služe samo za punjenje.
- Prebaci pločicu u režim za upload: drži BOOT, kratko pritisni RST, pusti BOOT ([korak 4](4-kod.md#upload)).
- Zatvori Serial Monitor pre uploada, jer drži port zauzetim.

## Serial Monitor je prazan

- Pritisni RST na pločici: početne poruke se ispišu pre nego što se monitor poveže.
- Brzina mora biti 115200. Projekat je već tako podešen u `platformio.ini`.

## `RC522 ne odgovara`

Verzija 0x00 ili 0xFF znači da ESP ne dobija odgovor od čitača.

- Proveri šest žica po tabeli iz [koraka 3](3-lemljenje.md#3-rc522). Najčešće su zamenjene MOSI i MISO.
- RC522 mora da dobija 3,3 V, a RST mora biti spojen na 3.3V.
- Proveri da nema kalaja između susednih pinova.

## Pesma seče ili se figurica ne vidi kroz poklopac

Ako pesma svira par sekundi, stane na trenutak i nastavi, radio na trenutak izgubi figuricu, pa misli da je skinuta. U Serial Monitoru se tada ponavlja `figurica skinuta` i odmah posle toga figurica ponovo spuštena.

Na prototipu je uzrok bilo pojačanje prijemnika u RC522. Na najjačem pojačanju (48 dB) čitač pojača i smetnje, pa je kroz zatvoren poklopac promašio 84 % čitanja. Na 33 dB nije promašio nijedno. Zato je u kodu podrazumevano 33 dB (`RFID_RX_GAIN = 4`).

Ako kod tebe ipak seče ili čitač ne vidi figuricu, nađi pojačanje koje radi:

1. Spusti figuricu, sačekaj 20 sekundi i pošalji `rfid`. Ispiše se koliko je puta čitač pitao figuricu, koliko puta ju je promašio i koliko puta je proglasio skinutom.
2. Pošalji `gain 5` (ili 3, 6...), sačekaj opet 20 sekundi i pošalji `rfid`. Brojevi se sabiraju od paljenja, pa gledaj za koliko su porasli.
3. Kad nađeš vrednost bez promašaja, upiši je u `RFID_RX_GAIN` u `Config.h` i ponovo učitaj kod. `gain` važi samo do gašenja.

| Vrednost | Pojačanje |
|---|---|
| 0 ili 2 | 18 dB |
| 1 ili 3 | 23 dB |
| 4 | 33 dB |
| 5 | 38 dB |
| 6 | 43 dB |
| 7 | 48 dB |

Proveri i ovo:

- Kraj RC522 sa pinovima mora biti na strani USB-C otvora ([korak 6](6-sklapanje.md#8-rc522-na-poklopac)). Obrnuto okrenut, čitač ima antenu 2 cm sa strane.
- Nalepnica mora biti u sredini postolja, a figurica bez metala u osnovi.
- Žice ne smeju da prelaze preko RC522.

## Nema zvuka

- Pošalji `info`. Ako je broj fajlova na kartici 0 ili -1, kartica nije FAT32, nije dobro ubačena, ili pesme nisu u folderu `mp3` ([korak 5](5-pesme.md)).
- `greska 6 (nema fajla)` znači da na kartici nema pesme sa tim brojem.
- Zvučnik mora biti na SPK_1 i SPK_2, a ne na GND.
- Proveri jačinu: `vol 15`.

## Pušta se pogrešna pesma

- Pošalji `play 5`. Ako ne krene `0005.mp3`, tvoj DFPlayer pušta po redosledu kopiranja. Formatiraj karticu i kopiraj fajlove redom ([korak 5](5-pesme.md#redosled-kopiranja)).
- Pošalji `list` i proveri koji broj ima figurica.

## Zvuk stane na glasnim delovima pesme

Ako lampica na DFPlayeru zgasne ili u dnevniku piše `DFPlayer se restartovao`, napon padne kad zvučnik vuče najviše struje.

- Kondenzator 470 µF mora biti zalemljen tik uz DFPlayer ([korak 3](3-lemljenje.md#2-esp32-c3-i-dfplayer)).
- Žice za napajanje treba da budu duple.
- Step-up izmeren pod opterećenjem treba da daje 5,0 V.
- Smanji `VOLUME_MAX` u `Config.h`.

## Šum ili pucketanje u zvučniku

- Proveri da je otpornik 1 kΩ na žici ka RX DFPlayera.
- Dodaj keramički kondenzator 100 nF uz 470 µF kod DFPlayera.

## Dugme za jačinu

- Jačina se menja tek na svaki drugi škljocaj: u `Config.h` stavi `ENC_STEPS_PER_DETENT = 2`.
- Jačina raste na pogrešnu stranu: stavi `ENC_REVERSE = true`.
- Jačina se uopšte ne menja: srednja nožica enkodera (C) mora biti na masi, A na GPIO0, B na GPIO1.

## Baterija pokazuje 0 % ili oko 1 V

Pošalji `bat`. Ako napon nije između 3,0 i 4,2 V, delitelj nije dobro povezan: GPIO3 mora preko jednog otpornika od 100 kΩ ići na plus baterije posle prekidača, a preko drugog na masu ([korak 3](3-lemljenje.md#5-delitelj-za-merenje-baterije)). Dok delitelj ne radi, radio ne upozorava na praznu bateriju. Bateriju i dalje štiti TP4056, koji je isključi oko 2,4 V.

## Radio se sam restartuje

Na početku dnevnika piše razlog paljenja (`start:`). `pad napona (brownout)` znači da je napon pao toliko da se ESP restartovao. Rešenje je isto kao za [zvuk koji staje na glasnim delovima](#zvuk-stane-na-glasnim-delovima-pesme).

[Nazad na početak](../README.md)
