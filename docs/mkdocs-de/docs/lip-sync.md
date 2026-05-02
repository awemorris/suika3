Lip Synchronization
===================

Ein Bild für die Lippensynchronisation muss im Ordner "lip" gespeichert werden,
der sich im selben Verzeichnis wie das oder die Charakterbilder befindet.

* happy.png (Hauptdatei der Figur)
* lip/
    * happy.png (Datei für die Lippensynchronisation)

Ein Lippensynchronisationsbild besteht aus Rahmen mit Unterschieden für die Lippensynchronisation.
Ein Frame muss die gleiche Größe haben wie das Charakterbild.
Frames müssen horizontal von links nach rechts gespeichert werden.

Die Alpha-Werte an den Rändern müssen geglättet werden.
Bitte verwende in einem Bildbearbeitungsprogramm "Blur Selection" und "Delete Selection".

Das Lippensynchronisationsintervall kann in der Datei `project.txt` angegeben werden.

```
#
# Länge eines Lippensynchronisationsframes (Sekunden pro Frame)
#
character.lipsync.frame=0.3

#
# Lippensynchronisationshäufigkeit pro Zeichen
#
character.lipsync.chars=3
```
