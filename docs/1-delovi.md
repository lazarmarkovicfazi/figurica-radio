# 1. Delovi i alat

[← Nazad na početak](../README.md) · [Sledeći korak: štampa →](2-stampa.md)

Sve se nalazi na AliExpress-u i u većini prodavnica elektronike. U koloni „na šta da paziš“ su mere za koje je kućište napravljeno. Ako tvoj modul odstupa, kućište se lako prilagođava (vidi [korak 2](2-stampa.md#pre-štampe-uporedi-mere)).

## Elektronika

| Deo | Kom. | Na šta da paziš |
|---|---|---|
| ESP32-C3 SuperMini | 1 | Mala pločica sa USB-C konektorom. Ležište u kutiji je 23,2 × 18,6 mm. |
| DFPlayer Mini | 1 | MP3 modul sa slotom za microSD karticu. Ležište je 21,2 × 21,2 mm. |
| RC522 RFID čitač, 13,56 MHz | 1 | Plava pločica 60 × 40 mm. Obično dolazi u kompletu sa jednom karticom i jednim privezkom. |
| NFC nalepnice | 1 po figurici | Okrugle, prečnika **do 26 mm**, tip NTAG213, NTAG215 ili MIFARE Classic. Moraju biti 13,56 MHz. Nalepnice od 125 kHz (EM4100) RC522 ne vidi. |
| TP4056 punjač sa zaštitom, USB-C | 1 | Uzmi verziju sa **šest** priključaka: IN+, IN−, B+, B−, OUT+, OUT−. Verzija sa samo četiri nema zaštitu baterije. Pločica je 28 × 17,4 mm. |
| Step-up modul 5 V | 1 | Podiže napon baterije na 5 V. Treba da izdrži bar 2 A, a najbolje je da ima trimer za podešavanje izlaza. Ležište je 50,6 × 28,6 mm, a modul sme da bude visok do 15 mm. |
| Li-ion ćelija 18650 | 1 | **Bez** ugrađene zaštitne pločice, dužine oko 65 mm. Ćelije sa zaštitom su duže i ne staju. Uzmi ćeliju poznatog proizvođača (Samsung, LG, Sony/Murata, Molicel). |
| Kontakti za 18650 | 1 par | Ravna pločica za plus i opruga za minus, pločice 13 × 11 mm. |
| Rotacioni enkoder EC11 sa tasterom | 1 | Goli enkoder sa pet nožica, **ne** modul KY-040. Osovina dužine 20 mm, sa ravnim delom („D“) ili nazubljena. |
| Prekidač klackalica KCD1 | 1 | Mali, sa dve nožice. Otvor u kutiji je 19,3 × 12,4 mm. |
| Zvučnik Ø40 mm | 1 | Tanak (do 5 mm), 4–8 Ω, 2–3 W. |
| microSD kartica | 1 | Do 32 GB. Veće kartice DFPlayer ne čita. |

## Sitni delovi

| Deo | Kom. | Čemu služi |
|---|---|---|
| Otpornik 1 kΩ | 1 | na žici od ESP-a ka DFPlayeru, smanjuje šum u zvučniku |
| Otpornik 100 kΩ | 2 | delitelj napona za merenje baterije |
| Elektrolitski kondenzator 470 µF, 10 V ili više | 1 | kod DFPlayera, drži napon na glasnim delovima pesme |
| Keramički kondenzator 100 nF | 2 | jedan kod DFPlayera, jedan kod RC522 |
| Heat insert M3, spoljni prečnik 4,6 mm, dužina 5,7 mm | 4 | navoji u stubovima kutije, za šrafove gornje ploče |
| Šraf M3 × 7 mm | 7 | 4 za gornju ploču, 2 za stezaljke RC522, 1 iza TP4056; glava do Ø7 mm |
| UTP kabl Cat5e ili Cat6, puni bakar | oko 1 m | sve žice u kutiji (vidi [korak 3](3-lemljenje.md#žice-iz-utp-kabla)) |
| Termoskupljajući bužir, tanak | malo | preko otpornika i lemnih spojeva |
| Dvostrana lepljiva traka ili vrući lepak | malo | moduli na dnu kutije |
| Kablovska vezica, uska | 1 | opciono, dodatno drži bateriju |

Ako imaš heat inserte druge veličine, npr. 5 × 4 mm, u `kuciste.py` promeni `INSERT_HOLE` na 4.4 i `INSERT_L` na 4.0, pa ponovo napravi STL (vidi [korak 2](2-stampa.md#izmena-kućišta)).

## Za štampu

- 3D štampač sa pločom od bar 120 × 90 mm (najveći deo je 115 × 86 mm).
- PETG ili PLA. Tačnu količinu ti pokaže slajser kad učitaš STL fajlove.

## Alat

- Lemilica sa tankim vrhom i kalaj (tinol) sa kolofonijom.
- **Multimetar.** Bez njega ne možeš da podesiš step-up na 5,0 V.
- Kliješta za skidanje izolacije, ili oštar skalpel.
- Mali odvijač za trimer na step-upu.
- Upaljač ili fen za termoskupljajući bužir.
- Računar i USB-C kabl **za podatke**. Neki kablovi služe samo za punjenje i sa njima računar ne vidi pločicu.

[Sledeći korak: štampa →](2-stampa.md)
