Anime
=====

Anime ist eine Funktion zum Abspielen ebenenbasierter Animationen über das Anime-Tag.

## Anime-Datei

Eine Anime-Datei ist eine Textdatei, die Sequenzen von Ebenentransformationen beschreibt.

## Sequenz

Um das Meldungsfeld in einer Sekunde um 100 Pixel nach rechts zu verschieben, schreiben Sie die folgende Sequenz in eine Anime-Datei.

```
# Ein Block beschreibt eine Sequenz zur Animation.
# Der Name eines Blocks kann beliebig sein und hat keinerlei Auswirkungen.
move {
    # Dies ist ein Ebenenspezifizierer.
    layer: msg;

    # Dies sind Zeitspezifizierer. (in Sekunden)
    start: 0.0;
    end: 1.0;

    # Dies sind Ursprungspositionsspezifizierer. „r0“ bedeutet relative „0“.
    from-x: r0;
    from-y: r0;

    # Dies ist ein Ursprungs-Alpha-Spezifizierer.
    from-a: 255;

    # Dies sind Endpositionsspezifizierer. „r100“ bedeutet relativ „100“.
    to-x: r100;
    to-y: r0;

    # Dies ist ein endgültiger Alpha-Spezifizierer.
    to-a: 255;
}
```

## Schichtstruktur

Das Folgende ist unsere Schichtenstruktur in der Reihenfolge von unten nach oben.

| Layername | Beschreibung |
|------------------|--------------------------------------------|
| bg | Hintergrund |
| bg2 | zweiter Hintergrund (für nahtloses Scrollen) |
| efb1 | Wirkung auf Rücken 1 |
| efb2 | Wirkung auf Rücken 2 |
| efb3 | Wirkung auf den Rücken 3 |
| efb4 | Wirkung auf Rücken 4 |
| chb | Charakter hinten in der Mitte |
| Chb-Auge | Charakter hinten in der Mitte |
| Chb-Lippe | Charakter hinten in der Mitte |
| chl | Zeichen links |
| chl-auge | Zeichen links |
| chl-Lippe | Zeichen links |
| chlc | Zeichen in der linken Mitte |
| chlc-Auge | Zeichen in der linken Mitte |
| chlc-Lippe | Zeichen in der linken Mitte |
| chr | Charakter rechts |
| Chr-Auge | Charakter rechts |
| Chr-Lippe | Charakter rechts |
| Chr | Zeichen in der rechten Mitte |
| Chrc-Auge | Zeichen in der rechten Mitte |
| Chrc-Lippe | Zeichen in der rechten Mitte |
| eff1 | Wirkung auf Vorderseite 1 |
| eff2 | Wirkung auf Vorderseite 2 |
| eff3 | Wirkung auf Vorderseite 3 |
| eff4 | Wirkung auf Vorderseite 4 |
| msgbox | Meldungsfeld |
| Namensfeld | Namensfeld |
| wähle1-idle | Wählen Sie Feld 1 (inaktiv) |
| Choose1-hover | Wählen Sie Feld 1 (bewegen Sie den Mauszeiger) |
| wähle2-idle | Wählen Sie Feld 2 (inaktiv) |
| Choose2-Hover | Wählen Sie Feld 2 (bewegen Sie den Mauszeiger) |
| wähle3-idle | Wählen Sie Feld 3 (inaktiv) |
| Choose3-hover | Wählen Sie Feld 3 (bewegen Sie den Mauszeiger) |
| wähle4-idle | Wählen Sie Feld 4 (inaktiv) |
| Choose4-hover | Wählen Sie Feld 4 (bewegen Sie den Mauszeiger) |
| wähle5-idle | Wählen Sie Feld 5 (inaktiv) |
| Choose5-hover | Wählen Sie Feld 5 (bewegen Sie den Mauszeiger) |
| wähle6-idle | Wählen Sie Feld 6 (inaktiv) |
| Choose6-hover | Wählen Sie Feld 6 (bewegen Sie den Mauszeiger) |
| wähle7-idle | Wählen Sie Feld 7 (inaktiv) |
| Choose7-hover | Wählen Sie Feld 7 (bewegen Sie den Mauszeiger) |
| wähle8-idle | Wählen Sie Feld 8 (inaktiv) |
| Choose8-Hover | Wählen Sie Feld 8 (bewegen Sie den Mauszeiger) |
| CHF | Charaktergesicht |
| CHF-Auge | Charaktergesicht |
| CHF-Lippe | Charaktergesicht |
| klicken | Klick-Animation |
| Auto | Banner für den automatischen Modus |
| überspringen | Banner für den Überspringmodus |
| Text 1 | Textebene 1 |
| Text2 | Textebene 2 |
| text3 | Textebene 3 |
| Text4 | Textebene 4 |
| Text5 | Textebene 5 |
| Text6 | Textebene 6 |
| Text7 | Textebene 7 |
| Text8 | Textebene 8 |
| GUI-Button-1 | GUI-Schaltflächen-ID 1 |
| GUI-Button-2 | GUI-Schaltflächen-ID 2 |
| GUI-Button-3 | GUI-Schaltflächen-ID 3 |
| GUI-Button-4 | GUI-Tasten-ID 4 |
| GUI-Button-5 | GUI-Schaltflächen-ID 5 |
| GUI-Button-6 | GUI-Schaltflächen-ID 6 |
| GUI-Button-7 | GUI-Schaltflächen-ID 7 |
| GUI-Button-8 | GUI-Schaltflächen-ID 8 |
| GUI-Button-9 | GUI-Schaltflächen-ID 9 |
| GUI-Button-10 | GUI-Schaltflächen-ID 10 |
| GUI-Button-11 | GUI-Schaltflächen-ID 11 |
| GUI-Button-12 | GUI-Schaltflächen-ID 12 |
| GUI-Button-13 | GUI-Schaltflächen-ID 13 |
| GUI-Button-14 | GUI-Schaltflächen-ID 14 |
| GUI-Button-15 | GUI-Schaltflächen-ID 15 |
| GUI-Button-16 | GUI-Schaltflächen-ID 16 |
| GUI-Button-17 | GUI-Schaltflächen-ID 17 |
| GUI-Button-18 | GUI-Schaltflächen-ID 18 |
| GUI-Button-19 | GUI-Schaltflächen-ID 19 |
| GUI-Button-20 | GUI-Schaltflächen-ID 20 |
| GUI-Button-21 | GUI-Schaltflächen-ID 21 |
| GUI-Button-22 | GUI-Schaltflächen-ID 22 |
| GUI-Button-23 | GUI-Schaltflächen-ID 23 |
| GUI-Button-24 | GUI-Schaltflächen-ID 24 |
| GUI-Button-25 | GUI-Schaltflächen-ID 25 |
| GUI-Button-26 | GUI-Schaltflächen-ID 26 |
| GUI-Button-27 | GUI-Schaltflächen-ID 27 |
| GUI-Button-28 | GUI-Schaltflächen-ID 28 |
| GUI-Button-29 | GUI-Schaltflächen-ID 29 |
| GUI-Button-30 | GUI-Schaltflächen-ID 30 |
| GUI-Button-31 | GUI-Schaltflächen-ID 31 |
| GUI-Button-32 | GUI-Schaltflächen-ID 32 |

## Skalierung und Rotation

```
# Vergrößern und drehen Sie die Ebene `effect1` in 3 Sekunden auf 2.0x und 360 Grad.
test1 {
    layer: effect1;
 
    start: 0.0;
    end: 3.0;

    from-x: 0;
    from-y: 400;
    from-a: 255;

    to-x: 0;
    to-y: 400;
    to-a: 255;

    # Skalierungs- und Rotationsursprung
    center-x: 600;
    center-y: 100;

    # Skalierungsfaktoren
    from-scale-x: 1.0;
    from-scale-y: 1.0;
    to-scale-x: 2.0;
    to-scale-y: 2.0;
 
    # Drehung (+ für rechts, - für links)
    from-rotate: 0.0;
    to-rotate: -360;
}

# Umkehren.
test2 {
    layer: effect1;

    start: 3.0;
    end: 6.0;

    from-x: 0;
    from-y: 400;
    from-a: 255;

    to-x: 0;
    to-y: 400;
    to-a: 255;

    center-x: 600;
    center-y: 100;

    from-scale-x: 2.0;
    to-scale-x: 1.0;

    from-scale-y: 2.0;
    to-scale-y: 1.0;

    from-rotate: -360;
    to-rotate: 0;
}
```
