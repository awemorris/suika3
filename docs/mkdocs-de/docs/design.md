Suika3 Design
=============

## Geschichtetes Komponentenmodell

Suika3 ist kein monolithisches Design, sondern in kleine Bibliotheken und Teile aufgeteilt und besitzt insgesamt eine hierarchische Struktur, die aufeinander aufbaut.

- Jede Schicht implementiert nur eine Funktion.
- Jede Schicht stellt der Schicht eine Ebene darüber eine öffentliche C-API bereit.
- Jede Schicht darf nur die öffentliche C-API der Schicht eine Ebene darunter verwenden.

Diese Art von Struktur wird in Suika3 als "Layered Component Model" bezeichnet.

Der Vorteil dieses Ansatzes besteht darin, dass Klassen in objektorientierten Sprachen wie C++ viele-zu-viele-Abhängigkeiten mit ihrer inhärenten Komplexität berücksichtigen müssen, während das Layered Component Model nur die Beziehungen zur jeweils darüber- und darunterliegenden Schicht beachten muss, was Entwurf, Implementierung, Änderung und Erweiterung vereinfacht.
Eine Full-Stack-Spiel-Engine ist komplexe und große Software, doch wenn sie über Eins-zu-eins-Beziehungen einfacher Komponenten zusammengesetzt wird, lässt sie sich als Ganzes leichter bauen, was zu besserer Portabilität führt.

Der Grund für die verbesserte Portabilität ist, dass die unterste Schicht eine "Hardware-Abstraktionsschicht" ist, die Betriebssystemunterschiede abfedert, und die darüberliegenden Schichten ohne Rücksicht auf OS-Unterschiede entworfen und implementiert werden können.

```
+-----------------------------------+
| VN Engine (Suika3)                |
+-----------------------------------+
| 2D Game Engine (Playfield Engine) |
+-----------------------------------+
| Scripting Language (NoctLang)     |
+-----------------------------------+
| Hardware Abstraction Layer        |
| (StratoHAL)                       |
+-----------------------------------+
```
