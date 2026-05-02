Systemmenü
===========

## SysBtn

Suika3 verfügt über eine Hamburger-Menüschaltfläche, die sich typischerweise in der linken oberen Ecke des Bildschirms befindet.
Intern wird diese Schaltfläche als `System Button (SysBtn)` bezeichnet.

Um die Richtlinien des App Stores einzuhalten, vermeidet Suika3 kleine
PC-artige Schaltflächen rund um das Textfenster und verfolgt stattdessen
einen Mobile-First-Ansatz. Der SysBtn besteht aus zwei Bildern und den
dazugehörigen Animationen. Für ein nahtloses Nutzungserlebnis ist die
Schaltfläche reaktiv: Sie erscheint bei Mausbewegung oder Berührung und
verschwindet nach einigen Sekunden Inaktivität automatisch wieder.
Während dieses Verhalten fest für die Store-Konformität eingebaut ist,
kann der SysBtn vollständig deaktiviert werden, etwa für Demos oder den
Kiosk-Modus, indem `sysbtn.enable=false` in der Konfiguration gesetzt wird.

Auch wenn das Fehlen von Schaltflächen rund um das vertraute Textfenster
anfangs ungewohnt wirken mag, hoffen wir, dass nachvollziehbar ist, dass
diese Entwicklung für die Anpassung von Visual Novels an moderne
Mobilplattformen notwendig ist.

Siehe auch:
- `config.ini` (Suche nach `sysbtn`)
- `system/sysbtn/` (im Beispielspiel)

## SysMenu

Ein Klick auf den SysBtn öffnet eine GUI mit dem Namen `System Menu
(SysMenu)`. Das SysMenu ist vollständig anpassbar und kann so konfiguriert
werden, dass es wesentliche Funktionen wie Speichern, Laden, Auto-Modus,
Skip-Modus, Verlauf und Konfiguration enthält.

Siehe auch:
- `system/sysmenu/` (im Beispielspiel)

## Speicher-/Ladebildschirme

Speicher- und Ladebildschirme sind über GUI-Dateien vollständig anpassbar.

Siehe auch:
- `system/save/` (im Beispielspiel)
- `system/load/` (im Beispielspiel)

## Schaltflächen für Auto- und Skip-Modus

Die Auto- und Skip-Schaltflächen verwenden spezialisierte GUI-Schaltflächentypen.
Diese Schaltflächen aktivieren beim Anklicken ihre jeweiligen Modi,
also Auto-Modus oder Skip-Modus.

Siehe auch:
- `system/sysmenu/` (im Beispielspiel)

## Verlauf-Bildschirm

Der Verlaufsbildschirm ist über eine GUI-Datei vollständig anpassbar.

Siehe auch:
- `system/history/` (im Beispielspiel)

## Konfigurationsbildschirm

Der Konfigurationsbildschirm ist über eine GUI-Datei vollständig anpassbar.

Der Konfigurationsbildschirm im Beispielspiel enthält:
- Regler für BGM-, Soundeffekt- und Sprachlautstärke
- Sprachumschaltung (EN/JP)
- Anpassung von Text- und Auto-Modus-Geschwindigkeit
- Vorschau der Textgeschwindigkeit

Außerdem kannst du implementieren:
- Master-Lautstärke
- Lautstärke pro Figur
- Sprachumschaltung (für alle unterstützten Sprachen)
- Eigene Schaltflächen

Siehe auch:
- `system/config/` (im Beispielspiel)
