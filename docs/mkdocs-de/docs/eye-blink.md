Eye Blinking
============

Ein Bild für das Augenblinzeln muss im Ordner "eye" gespeichert werden,
der sich im selben Verzeichnis wie das oder die Charakterbilder befindet.

- happy.png (Hauptdatei der Figur)
- eye/
    - happy.png (Datei für das Augenblinzeln)

Ein Augenblinzelbild besteht aus Rahmen mit Unterschieden für das Augenblinzeln.
Ein Frame muss die gleiche Größe haben wie das Charakterbild.
Frames müssen horizontal von links nach rechts gespeichert werden. Das tatsächliche Bild ist im Beispielspiel zu sehen.

Die Alpha-Werte an den Rändern müssen geglättet werden.
Bitte verwende in einem Bildbearbeitungsprogramm "Blur Selection" und "Delete Selection".

Das Augenblinzelintervall kann in der Datei `config.ini` angegeben werden.
Die Intervalle werden leicht zufällig variiert, und manchmal treten Doppelblinzeln auf.

```
#
# Augenblinzelintervall (Sekunden)
#
character.eyeblink.interval=4.0

#
# Länge eines Augenblinzelframes (Sekunden pro Frame)
#
character.eyeblink.frame=0.15
```
