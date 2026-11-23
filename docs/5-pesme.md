# 5. Pesme na SD kartici

[← Kod](4-kod.md) · [Sledeći korak: sklapanje →](6-sklapanje.md)

DFPlayer je jednostavan modul i ne razume imena pesama. Svaka pesma ima svoj broj, a broj je ime fajla.

## Kartica

- microSD do 32 GB.
- Formatirana kao **FAT32**. DFPlayer ne čita exFAT, koji je podrazumevan za veće kartice. U Windowsu: desni klik na karticu → *Format* → *File system: FAT32*.

## Fajlovi

U korenu kartice napravi folder **`mp3`**. Pesme u njemu se zovu po broju, uvek sa četiri cifre:

```
mp3/
  0001.mp3
  0002.mp3
  0003.mp3
  ...
  0100.mp3
  0998.mp3   opciono: sve pesme su zauzete, a spuštena je nova figurica
  0999.mp3   opciono: baterija je prazna, vreme je za punjač
```

Fajlovi 0998 i 0999 su sistemski zvuci. Možeš da snimiš svoj glas („baterija je prazna“) ili kratak zvuk. Ako ih nema, radio u tim slučajevima samo ćuti.

Napravi sebi i spisak koji broj je koja pesma. Na kartici se vide samo brojevi.

## Redosled kopiranja

Neke kopije DFPlayera ne puštaju fajl po imenu, nego po redosledu kojim je kopiran na karticu. Da bi broj uvek odgovarao imenu:

1. Formatiraj karticu.
2. Kopiraj fajlove na praznu karticu redom po imenu: prvo 0001, pa 0002 i tako dalje. Najsigurnije je jedan po jedan, ili u manjim grupama sortiranim po imenu.
3. Kasnije nove pesme dodaj samo na kraj, sa većim brojem.

Proveri u Serial Monitoru: `play 5` mora da pusti baš `0005.mp3`.

Na macOS-u, Finder na karticu dodaje skrivene fajlove čije ime počinje sa `._` i DFPlayer ih broji kao pesme. Posle kopiranja ih obriši iz Terminala:

```bash
dot_clean -m /Volumes/IME_KARTICE
```

[Sledeći korak: sklapanje →](6-sklapanje.md)
