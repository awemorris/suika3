Systemanforderungen für Suika3
===========================================

## 1. Überblick

Suika3 ist eine leistungsstarke Skript-Runtime, die für Visual Novels
(VN) und 2D-Spiele optimiert ist. Sie bietet eine mehrschichtige
DSL-Umgebung, die einfache Bedienung mit professioneller
Erweiterbarkeit ausbalanciert.

## 2. Kernkomponenten (Der DSL-Stack)

Suika3 unterstützt Creator mit vier spezialisierten Sprachen:

- NovelML (Tag-basierte DSL): Eine einfache Auszeichnungssprache mit
  `[]`-Tags für die schnelle Entwicklung von VN-Szenarien.

- Anime (Animations-DSL): Ein eigenes System für schichtenbasierte
  Rasterbildanimationen mit Steuerung von Affin-Transformationsfolgen.

- GUI (UI/UX-DSL): Ein flexibles Werkzeugset zum Erstellen interaktiver
  Bildschirme mit für VN-Anforderungen optimierten Schaltflächen.

- Ray (Allzweck-Skripting): Eine leistungsstarke Skriptsprache mit
  VN-API.
    - Anpassung: Definiere eigene NovelML-Tags.
    - Leistung: Auf dem PC JIT-kompiliert für schnelle Iteration;
      AOT-kompiliert zu nativen Binärdateien für iOS-Konformität.
    - Low-Level-Zugriff: Direkte Hooks in die Suika3-Core-C-APIs.

## 3. Wichtige Ziele und Designphilosophie

- Mobile-First-Erlebnis: Entwickelt mit der Überzeugung, dass
  Smartphones ein primäres Endgerät sind. PC-zentrierte UI/UX wird
  zugunsten eines nativen Mobile-Gefühls vermieden.

- Kompatibilität mit Store-Veröffentlichungen: Vollständig konform mit
  den Richtlinien von iOS- und Android-Stores durch AOT-Kompilierung und
  responsives Design.

- Hohe Portabilität:
    - Stufe 1: iOS, Android, HarmonyOS NEXT, Windows, macOS, Linux
    - Stufe 2: Spielekonsolen
    - Stufe 3: Chromebook, Wasm (WebAssembly)

- Über Visual Novels hinaus: Obwohl auf VN ausgerichtet, ermöglicht
  die zugrunde liegende 2D-Basis Genre-Mischungen (z. B. VN + RPG oder
  Action).

## 4. Nicht im Umfang / Einschränkungen

Um Portabilität und Leistung zu bewahren, schließt Suika3 ausdrücklich aus:

- Nur-PC-Funktionen: Suika3 ist kein Ersatz für ältere VN-Engines, die
  ausschließlich für den PC gedacht sind.

- Web-Deployment im großen Stil: Der Wasm-Port ist für Demos gedacht,
  nicht für die primäre Verteilung.

- 3D-Grafik: Der Fokus liegt derzeit auf 2D (zukünftige 3D-Unterstützung
  ist zusammen mit KI-gestützter Asset-Generierung geplant).

- Proprietäre Middleware: Keine Unterstützung für geschlossene
  Technologien wie Live2D, um maximale Engine-Portabilität sicherzustellen.

## 5. NovelML

## 6. Ray

## 7. Anime

## 8. GUI

## 9. Config
