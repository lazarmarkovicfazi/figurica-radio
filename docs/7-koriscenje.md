# 7. Korišćenje

[← Sklapanje](6-sklapanje.md) · [Sledeći korak: kad nešto ne radi →](8-problemi.md)

## Figurice

| Šta uradiš | Šta radio uradi |
|---|---|
| spustiš figuricu | pusti njenu pesmu |
| skineš figuricu | pauzira pesmu |
| vratiš istu figuricu u roku od 10 minuta | nastavi gde je stala |
| vratiš je kasnije | pusti pesmu ispočetka |
| spustiš drugu figuricu | pusti pesmu te figurice |
| pesma se završi, a figurica stoji | ćuti; klik na dugme je pusti ponovo |

## Dugme

| Šta uradiš | Šta radio uradi |
|---|---|
| okreneš | menja jačinu; radio je pamti i posle gašenja |
| klikneš | pauza ili nastavak |
| držiš 1,5 sekundu | pesma ispočetka |

## Kako figurica dobije pesmu

Radio čita samo serijski broj nalepnice. U nalepnicu se ništa ne upisuje, pa ne treba nikakva aplikacija.

Kad spustiš figuricu koju radio još ne zna, ona dobije **prvi slobodan broj** pesme: prva figurica pesmu 1 (`0001.mp3`), druga pesmu 2, i tako redom. Radio to pamti u svojoj memoriji, i posle gašenja i posle novog učitavanja koda.

Ako hoćeš da tačno određena figurica pušta tačno određenu pesmu, imaš dva načina:

- **Spuštaj figurice redom.** Kad prvi put koristiš radio, spusti figurice istim redom kojim su pesme na kartici.
- **Preimenuj pesmu.** Spusti figuricu, pa u Serial Monitoru komandom `list` vidi koji broj je dobila. Pesmu koju želiš snimi na karticu pod tim brojem.

## Brisanje memorije

Poveži ESP na računar i otvori Serial Monitor ([korak 4](4-kod.md#serial-monitor)):

- `list` pokazuje koja figurica ima koju pesmu.
- `forget 5` oslobađa pesmu 5. Figurica koja ju je imala postaje nova, i kad je sledeći put spustiš, dobiće prvi slobodan broj.
- `forget` briše sve figurice.

## Punjenje

Kabl utakni u USB-C na desnoj strani kutije. Na punjaču TP4056 crvena lampica znači da se baterija puni, a plava ili zelena da je puna. Baterija se puni i kad je radio isključen.

Kad baterija oslabi, radio, dok ništa ne svira, pusti `0999.mp3` („vreme je za punjač“), najviše jednom u pet minuta. Kad je skoro prazna, prestane da svira dok je ne napuniš. Za ovo mora da radi delitelj napona iz [koraka 3](3-lemljenje.md#5-delitelj-za-merenje-baterije).

## Promena pesama

Skini gornju ploču i poklopac (četiri šrafa), izvadi SD karticu iz DFPlayera i dodaj pesme po [koraku 5](5-pesme.md). Figurice zadržavaju svoje brojeve.

[Sledeći korak: kad nešto ne radi →](8-problemi.md)
