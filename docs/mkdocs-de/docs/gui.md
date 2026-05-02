GUI
===

## Einführung

GUI ist die UI/UX-Erstellungsfunktion von Suika3.

In Suika3 werden Schaltflächen in einem dedizierten GUI-Modus definiert und funktionieren synchron, d. h. der Aufruf einer GUI führt zu einem Klick auf die Schaltfläche oder zu einem Abbruch.

Asynchrone Rückrufe wie das Anzeigen von Schaltflächen während der Ausführung von Text-Tags werden absichtlich vermieden, da sie für Programmieranfänger schwer zu verstehen und zu verwalten sein können.

Eine GUI-Datei enthält eine Liste von Schaltflächendefinitionen. Jede Schaltfläche enthält einen Verhaltenstyp, Leerlauf- und Hover-Bilder sowie zusätzliche Eigenschaften. Schaltflächen werden Animationsebenen zugeordnet und Animationsdateien können ausgelöst werden, wenn sich der Status einer Schaltfläche ändert.

---

## GUI-Beispiel

```
# Abschnitt „Globale Einstellungen“.
global {
    fadein:       0.2;            # Fading-in time in seconds.
    fadeout:      0.2;            # Fading-out time in seconds.
}

# Ein Block beschreibt eine Schaltfläche.
# Der Name eines Blocks kann beliebig sein und hat keinerlei Auswirkungen.
button1 {
    # Layer-ID (`1`-`32`)
    # `1` bedeutet jeweils die oberste und `32` die unterste in den GUI-Ebenen.
    id: 1;
 
    # Verhaltenstyp (`goto` bedeutet, dass beim Klicken zu einer Beschriftung gewechselt wird.)
    type: goto;

    # Etikett zum Aufrufen.
    label: button1_clicked;

    # Position
    x: 39;
    y: 99;

    # `width:` und `height:` können weggelassen werden, da sie aus der Bildgröße abgeleitet werden können.

    # Bilder
    image-idle:  gui/item-idle.png;    # Shown when the button is not pointed.
    image-hover: gui/item-hover.png;   # Shown when the button is pointed.
    image-press: gui/item-press.png;   # Shown when the button is pressed.

    # Animationen
    anime-idle:  gui/item-idle.anime;   # Executed when the button state is changed to `idle`.
    anime-hover: gui/item-hover.anime;  # Executed when the button state is changed to `hover`.
    anime-press: gui/item-press.anime;  # Executed when the button is pressed.

    # Beachten Sie, dass `press` kein unabhängiger Status ist, sondern ein zusätzliches Flag zum Status `hover`.
    # Wenn ein `hover`-Anime ausgeführt wird, wird `idle`-Anime abgebrochen.
    # Und wenn `idle` Anime läuft, wird `hover` Anime abgebrochen.
    # Allerdings macht `press` Anime nichts rückgängig.
}
```

Im Abschnitt `global` können Sie Optionen für die GUI angeben.

Andere Abschnitte werden als Schaltflächendefinitionen interpretiert. Hier erstellt `button1` eine Schaltfläche an Position `(39, 99)`. Wenn auf die Schaltfläche geklickt wird, erfolgt ein Sprung namens `goto`.

---

## Schaltflächentypen

| Beschreibung | Bedeutung |
|---------------------------------|---------------------------------------------------|
| Typ: noaction; | Nicht anklickbares Bild. |
| Typ: gehe zu; | Springt beim Klicken zu einer Beschriftung. |
| Typ: GUI; | Springt beim Klicken zu einer GUI. |
| Typ: Mastervol; | Wird als Master-Lautstärkeregler angezeigt. |
| Typ: bgmvol; | Wird als Hintergrundmusik-Lautstärkeregler angezeigt. |
| Typ: sevol; | Wird als SE-Lautstärkeregler angezeigt. |
| Typ: Voicevol; | Wird als Schieberegler für die Sprachlautstärke angezeigt. |
| Typ: Charaktervol; | Wird als Schieberegler für die Zeichenlautstärke angezeigt. |
| Typ: Textgeschwindigkeit; | Wird als Schieberegler für die Textgeschwindigkeit angezeigt. |
| Typ: Autospeed; | Wird als Geschwindigkeitsregler für den automatischen Modus angezeigt. |
| Typ: Vorschau; | Wird als Textvorschaubereich angezeigt. |
| Typ: Vollbild; | Wird als Schaltfläche für den Vollbildmodus angezeigt. |
| Typ: Fenster; | Wird als Fenstermodus-Taste angezeigt. |
| Typ: Standard; | Setzt die Einstellungen zurück, wenn sie gedrückt wird. |
| Typ: Savepage; | Wird als save/load-Seitenschaltfläche angezeigt. |
| Typ: Speichern; | Wird als Speicherplatz angezeigt. |
| Typ: laden; | Wird als Ladeschacht angezeigt. |
| Typ: Auto; | Wird als Schaltfläche für den automatischen Modus angezeigt. |
| Typ: überspringen; | Wird als Schaltfläche zum Überspringen des Modus angezeigt. |
| Typ: Titel; | Wird als Zurück-zum-Titel-Button angezeigt. |
| Typ: Geschichte; | Wird als Verlaufsschaltfläche angezeigt. |
| Typ: HistoryScroll; | Wird als vertikaler Verlaufs-Scroll-Schieberegler angezeigt. |
| Typ: HistoryScroll-Horizontal; | Wird als horizontaler Verlaufs-Scroll-Schieberegler angezeigt. |
| Typ: Abbrechen; | Wird als Abbrechen-Schaltfläche angezeigt. |
| Typ: Namevar; | Wird als Bereich zur Vorschau eines Namensvariablenwerts angezeigt. |
| Typ: char; | Wird als Schaltfläche zur Eingabe eines Zeichens angezeigt. |
| Typ: Sprache | Ändern Sie die Sprache, wenn Sie darauf klicken. |

### `noaction`

Ein nicht anklickbares Bild.

```
background {
    type: noaction;
    x: 0;
    y: 0;
    image-idle: idle.png;
}
```

### `goto`

Eine anklickbare Schaltfläche. Wenn darauf geklickt wird, springt die Tag-Ausführung zu einer durch den Parameter `label:` angegebenen Bezeichnung.

```
button1 {
    type: goto;
    label: label_clicked_button1;
    x: 0;
    y: 0;
    image-idle:  idle.png;
    image-hover: hover.png;
}
```

### `gui`

Eine anklickbare Schaltfläche. Wenn Sie darauf klicken, wird die GUI-Ausführung an eine neue GUI verkettet, die durch den Parameter `file:` angegeben wird.

```
button1 {
    type: gui;
    label: gui2.gui;
    x: 0;
    y: 0;
    image-idle:  idle.png;
    image-hover: hover.png;
}
```

### `mastervol`

Ein Schieberegler zum Einstellen der Gesamtlautstärke.

```
slider1 {
    type: mastervol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `bgmvol`

Ein Schieberegler zum Einstellen der Hintergrundmusiklautstärke.

Dieser Schieberegler legt das Volumen fest, das in der globalen Sicherungsdatendatei gespeichert werden soll. Dies unterscheidet sich von dem Volumen, das durch das Tag `[volume]` festgelegt und in den lokalen Speicherdaten gespeichert wird.

```
slider1 {
    type: bgmvol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `sevol`

Ein Schieberegler zum Einstellen der SE-Lautstärke.

Dieser Schieberegler legt das Volumen fest, das in der globalen Sicherungsdatendatei gespeichert werden soll. Dies unterscheidet sich von dem Volumen, das durch das Tag `[volume]` festgelegt und in den lokalen Speicherdaten gespeichert wird.

```
slider1 {
    type: bgmvol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `voicevol`

Ein Schieberegler zum Einstellen der SE-Lautstärke.

Dieser Schieberegler legt das Volumen fest, das in der globalen Sicherungsdatendatei gespeichert werden soll. Dies unterscheidet sich von dem Volumen, das durch das Tag `[volume]` festgelegt und in den lokalen Speicherdaten gespeichert wird.

```
slider1 {
    type: bgmvol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `charactervol`

Ein Schieberegler zum Festlegen der Lautstärke pro Zeichen.

Der Zeichenindex wird vom Parameter `index:` übergeben. Index 0 steht für nicht definierte Zeichen und 1-32 für definierte Zeichen.

```
slider1 {
    type: charactervol;
    index: 1;  # Character Index
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `textspeed`

Ein Schieberegler zum Einstellen der Textgeschwindigkeit.

```
slider1 {
    type: textspeed;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `autospeed`

Ein Schieberegler zum Einstellen der Geschwindigkeit im Automatikmodus.

```
slider1 {
    type: textspeed;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `preview`

Ein Textbereich zur Vorschau der Schriftart, Sprache und Geschwindigkeit.

```
preview1 {
    type: preview;
    x: 0;
    y: 0;
    image-idle: idle.png;
}
```

### `fullscreen`

Eine anklickbare Schaltfläche. Wenn Sie darauf klicken, wechselt die Engine in den Vollbildmodus.

```
fullscreen1 {
    type: fullscreen;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-disable: disable.png;  # For when in the full screen mode.
}
```

### `window`

Eine anklickbare Schaltfläche. Wenn Sie darauf klicken, wechselt die Engine in den Fenstermodus.

```
window1 {
    type: window;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-disable: disable.png;  # For when in the windowed mode.
}
```

### `default`

Eine anklickbare Schaltfläche. Wenn Sie darauf klicken, werden alle Einstellungen zurückgesetzt.

```
reset1 {
    type: default;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `savepage`

Eine anklickbare Schaltfläche. Wenn Sie darauf klicken, wird die Speicherbildschirmseite umgeschaltet.

```
savepage1 {
    type: savepage;
    index: 0; # Page index (0-)
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-active:  active.png;  # For when the page is active.
}
```

### `save`

Eine anklickbare Schaltfläche. Beim Anklicken wird ein Speichervorgang ausgeführt.

```
save1 {
    type: save;
    index: 0; # Index in apage. actual save slot = page * saveslots + index

    x: 0;
    y: 0;

    image-idle: system/save/save-item-idle.png;
    image-hover: system/save/save-item-hover.png;

    index-x:   10;   # Number
    index-y:   0;
    date-x:    60;   # Date
    date-y:    0;
    thumb-x:   27;   # Thumbnail
    thumb-y:   77;
    chapter-x: 260;  # Chapter title
    chapter-y: 48;
    msg-x:     260;  # Last message
    msg-y:     96;
}
```

### `load`

Eine anklickbare Schaltfläche. Wenn darauf geklickt wird, wird ein Ladevorgang ausgeführt.

```
load1 {
    type: load;
    index: 0; # Index in apage. actual save slot = page * saveslots + index

    x: 0;
    y: 0;

    image-idle: system/load/load-item-idle.png;
    image-hover: system/load/load-item-hover.png;

    index-x:   10;   # Number
    index-y:   0;
    date-x:    60;   # Date
    date-y:    0;
    thumb-x:   27;   # Thumbnail
    thumb-y:   77;
    chapter-x: 260;  # Chapter title
    chapter-y: 48;
    msg-x:     260;  # Last message
    msg-y:     96;
}
```

### `auto`

Eine anklickbare Schaltfläche. Wenn Sie darauf klicken, wird der Auto-Modus gestartet.

```
auto1 {
    type: auto;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `skip`

Eine anklickbare Schaltfläche. Wenn Sie darauf klicken, wird der Skip-Modus gestartet.

```
skip1 {
    type: skip;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `title`

Eine anklickbare Schaltfläche. Wenn Sie darauf klicken, kehrt die Engine zur angegebenen NovelML-Datei zurück.

```
title1 {
    type: title;
    file: title.novel;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `history`

Eine anklickbare Schaltfläche. Es zeigt ein Nachrichtenverlaufselement.

```
history {
    type: history;
    index: 0; # Index in a page
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;

    name-x: 20;   # Name
    name-y: 10;
    text-x:  20;  # Text (for when there is a name)
    text-y:  50;
    msg-x:  20;   # Message (for when no name)
    msg-y:  10;
}
```

### `historyscroll`

Eine vertikale Bildlaufleiste für den Verlaufsbildschirm.

```
scroll1 {
    type: historyscroll;
    x: 1200;
    y: 40;

    image-idle:  system/history/history-bar-idle.png;
    image-hover: system/history/history-bar-hover.png;
    image-knob:  system/history/history-bar-knob.png;
}
```

### `historyscroll-horizontal`

Eine horizontale Bildlaufleiste für den vertikalen Schreibverlaufsbildschirm.

```
scroll1 {
    type: historyscroll-horizontal;
    x: 40;
    y: 650;

    image-idle:  system/history/history-bar-idle.png;
    image-hover: system/history/history-bar-hover.png;
    image-knob:  system/history/history-bar-knob.png;
}
```

### `cancel`

Eine anklickbare Schaltfläche. Wenn Sie darauf klicken, wird die GUI abgebrochen.

```
cancel1 {
    type: cancel;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `var`

Ein Textbereich zur Anzeige eines Variablenwerts. Es wird zur Namensbearbeitung verwendet.

```
var1 {
    type: var;
    variable: variable_name;
    x: 0;
    y: 0;
    image-idle:    idle.png;
}
```

### `char`

Eine Schaltfläche zum Anhängen eines Zeichens an eine Variable. Es wird zur Namensbearbeitung verwendet.

```
char1 {
    type: char;
    variable: variable_name; # Variable name.
    msg: A;                  # Text to append.
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `language`

Eine Schaltfläche zum Umschalten der Sprache.

```
language_english1 {
    type: language;
    langu: en;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-disable: disable.png; # For when English is active.
}
```

---

## Allgemeine Schaltflächenoptionen

| Beschreibung | Bedeutung |
|-----------------------|-------------------------------------------------------------|
| Typ: | Typ der Schaltfläche. |
| X: | X-Position der Schaltfläche. |
| y: | Y-Position der Schaltfläche. |
| Breite: | Breite der Schaltfläche. (Standardmäßig ist die Bildbreite im Leerlauf eingestellt.) |
| Höhe: | Höhe der Schaltfläche. |
| Punkte: | Soundeffekt, wenn auf die Taste gezeigt wird. |
| klicke: | Soundeffekt, wenn auf die Schaltfläche geklickt wird. |

### Punkte:

`pointse: btn-__TECH0__;` gibt an, dass eine Soundeffektdatei abgespielt wird, wenn der Mauszeiger auf die Schaltfläche eintritt.

### klicke:

`clickse: __TECH0__;` gibt an, dass eine Soundeffektdatei abgespielt wird, wenn auf die Schaltfläche geklickt wird.
