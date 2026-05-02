Suika3: Einführungsanleitung
============================

Willkommen bei Suika3! Diese Anleitung hilft dir, dein allererstes
Visual-Novel-Projekt in nur wenigen einfachen Schritten zu starten.

## 1. Installation

Bringen wir die Engine zum Laufen, damit du die Magie sehen kannst!

### Windows

- **Herunterladen und entpacken**
    - Lade [Suika3-SDK-Full.zip](https://github.com/awemorris/suika3/releases/latest/download/Suika3-SDK-Full.zip) herunter und entpacke es in einen Ordner deiner Wahl.
- **Starten**
    - Öffne den Ordner und starte `suika3.exe`, um das Beispielspiel zu öffnen!

### macOS

- **Herunterladen und entpacken**
    - Lade `Suika3-full.zip` herunter und entpacke es in einen Ordner deiner Wahl.
- **Disk-Image einbinden**
    - Wechsle zu `SDK/macos/` und öffne `Suika3.dmg`.
- **App-Bundle einrichten**
    - Kopiere die App `Suika3` aus dem DMG in denselben Ordner, in dem `suika3.exe` und der Datenordner liegen.
    - Hinweis: Das App-Bundle muss sich zusammen mit deinen Spieldaten befinden, damit es korrekt funktioniert.
- **Starten**
    - Doppelklicke auf die App `Suika3`, um das Beispielspiel zu starten!

### Linux

- **Herunterladen und entpacken**
    - Lade `Suika3-full.zip` herunter und entpacke es in ein Verzeichnis deiner Wahl.
- **Flatpak-Paket installieren**
    - Wechsle zu `SDK/linux/` und öffne `Suika3.flatpak` (oder führe `flatpak install --user Suika3.flatpak` aus).
    - Dadurch werden `.novel`- und `.ray`-Dateien mit der Suika3-Engine verknüpft.
- **Starten**
    - Öffne den entpackten Ordner und doppelklicke auf `start.novel`, um das Beispielspiel zu starten!

## 2. Integration in Visual Studio Code

Die VSCode-Integration ist unter Windows, macOS und Linux verfügbar!

Außerdem ist [NovelML-Helper](https://github.com/lalalll-lalalll/NovelML-Helper) für Syntaxhervorhebung verfügbar.

- Öffne den entpackten Ordner in `Visual Studio Code`.
- Klicke auf die Befehls-Palette.
- Klicke auf `Run Task`.
- Wähle aus:
    - `Suika3: Run` (oder `Ctrl+Shift+B`)
    - `Suika3: Create a package`
    - `Suika3: Build Android APK`
    - `Suika3: Build iOS IPA`
- Klicke auf `PROBLEMS`, wenn ein Fehler aufgetreten ist.

## 3. Deine Geschichte anpassen (`start.novel`)

Jetzt sorgen wir dafür, dass das Spiel genau das sagt, was du möchtest.

- **Öffnen:**
    - Finde die Datei `start.novel` in deinem Projektordner und öffne sie mit deinem bevorzugten Texteditor.
- **Bearbeiten:**
    - Füge am Anfang der Datei den folgenden Tag ein:
    ```
    [text text="Hallo, Welt! Das ist mein erstes Spiel."]
    ```
- **Testen:**
    - Speichere die Datei und starte Suika3 erneut.
    - Du solltest deine neue Nachricht auf dem Bildschirm sehen!

## 4. Den Bildschirm anpassen (`main.ray`)

Du kannst das Aussehen und die Wirkung deines Spielfensters ganz einfach ändern.

- **Auffinden:**
    - Öffne die Datei `main.ray` in deinem Editor.
- **Ändern:**
    - Suche den Abschnitt `func setup()`.
    - Hier kannst du die Auflösung und den Titel deines Fensters ändern:
    ```
    // Wird beim Öffnen des Fensters aufgerufen.
    func setup() {
        return {
            width:      1280,            // Die Breite deines Spiels
            height:     720,             // Die Höhe deines Spiels
            title:      "Mein erstes Spiel", // Der Titel deines Spiels
            fullscreen: false            // Für den Vollbildmodus auf true setzen
        };
    }
    ```

## 5. Unter der Haube (erweiterte Tipps)

Der untere Teil deiner Datei `main.ray` enthält die Kernlogik der Engine.
Diese Funktionen lässt man am besten unverändert, sofern man keine
erweiterte Anpassung vornimmt:

- `func start()`:
    - Wird einmal aufgerufen, wenn dein Spiel startet.
- `func update()`:
    - Läuft in jedem einzelnen Frame, um die Spiellogik zu verarbeiten.
- `func render()`:
    - Zeichnet nach Abschluss des Updates alles auf dem Bildschirm.

```
// Wird vor dem Spielstart aufgerufen.
func start() {
    // Hier Plugins laden.
    // Suika.loadPlugin("testplugin");

    // Die folgende Zeile nicht löschen.
    Suika.start();
}

// Wird vor dem Rendern eines Frames aufgerufen.
func update() {
    // Die folgende Zeile nicht löschen.
    Suika.update();
}

// Wird bei jedem Frame-Rendern aufgerufen.
func render() {
    // Die folgende Zeile nicht löschen.
    Suika.render();
}
```

> [!TIPS]
> Diese Funktionen sind der Kernmechanismus der `Playfield Engine`, die
> Suika3 antreibt. `Suika.start()`, `Suika.update()` und `Suika.render()`
> müssen an Ort und Stelle bleiben, damit das Spiel korrekt funktioniert.
