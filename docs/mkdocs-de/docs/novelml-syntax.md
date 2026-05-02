NovelML-Syntaxreferenz
========================

In diesem Dokument werden die grundlegende Syntax von NovelML und seine Ausführung erläutert.

Eine ausführliche Erläuterung der einzelnen Tags finden Sie in der „Suika3-Tag-Referenz“.

---

## 1. NovelML ist eine Liste von Engine-Befehlen, die „Tags“ genannt werden.

NovelML ist eine Liste von **Tags**. Ein Tag ist ein **Befehl für die Engine**.

- Jedes Tag weist die Engine an, etwas zu tun
- Tags werden **einzeln** in der Reihenfolge ausgeführt, in der sie erscheinen
- Nachdem ein Tag ausgeführt wurde, ist es „fertig“ und die „Ausführungsposition“ wird fortgesetzt

Wenn Sie dasselbe Tag erneut schreiben, wird es **bei jeder Ausführung** dasselbe tun.

---

## 2. Die Ausführung erfolgt von oben nach unten

NovelML wird Tag für Tag vom Anfang der Datei bis zum Ende ausgeführt.

- Die Ausführung erfolgt normalerweise vorwärts
- Es gibt nur eine aktuelle **Ausführungsposition**

```PlainText
[text text="Hello"]
[text text="World"]
```

In diesem Fall:

1. `Hello` wird angezeigt
2. Dann wird `World` angezeigt

---

## 3. Alles muss als Tags geschrieben werden

In NovelML muss **jede Zeile ein Tag sein**.

- Das gesamte Szenario wird mithilfe von Tags geschrieben
- Auch Text muss das Tag `[text]` verwenden

```PlainText
[text text="It's a beautiful day."]
```

---

## 4. Tags, die warten, und Tags, die nicht warten

Es gibt zwei Hauptarten von Tags:

- **Tags, die ausgeführt werden und sofort zum nächsten Tag wechseln**
- **Tags, die auf Benutzereingaben oder den Abschluss eines Vorgangs warten**

Zu den gängigen „Warten“-Tags gehören:

- `text` (wartet auf einen Klick)
- `click` (wartet auf einen Klick)
- `wait` (wartet eine angegebene Zeit)
- `video` (wartet, bis die Wiedergabe beendet ist)

Diese Tags unterbrechen die Ausführung **ohne besondere Syntax**.

### 4.1 Tags, die im Hintergrund (asynchron) ausgeführt werden

Einige Tags werden möglicherweise **im Hintergrund ausgeführt** (asynchron).

Typische Beispiele:

- `anime`
- `move`

Diese Tags:

- Erstellen und starten Sie eine Animation oder Bewegung
- Wenn `async="true"` angegeben ist, wartet die Engine nicht, bis der Vorgang abgeschlossen ist, und fährt sofort mit dem nächsten Tag fort

Aus diesem Grund werden möglicherweise Tags nach einem `async="true"`-Tag ausgeführt, **während die Animation noch abgespielt wird**.

Damit können Sie Dinge tun wie:

- Verschieben Sie einen Hintergrund oder ein Zeichen
- Während gleichzeitig Text angezeigt oder andere Effekte ausgeführt werden

---

## 5. Tags, die den Ausführungsfluss ändern

Normalerweise erfolgt die Ausführung von oben nach unten, aber einige Tags **ändern sich, wo die Ausführung fortgesetzt wird**.

### 5.1 Etiketten

```PlainText
[label start]
```

- Eine Beschriftung markiert eine benannte Position im Szenario
- Das Ausführen eines Labels allein bewirkt nichts

### 5.2 Sprünge

```PlainText
[goto name="start"]
```

---

## 6. Bedingte Verzweigung (if / elseif / else)

```PlainText
[if ...]
  ...
[elseif ...]
  ...
[else]
  ...
[endif]
```

- Nur der erste passende Block wird ausgeführt
- Nicht ausgewählte Blöcke werden **komplett übersprungen**
- Sie können `elseif` null oder mehrmals schreiben
- `else` ist optional

---

## 7. Variablen und Variablenerweiterung

### 7.1 Variablen festlegen

Verwenden Sie das Tag `[set]`, um eine Variable festzulegen.

```PlainText
[set name="player_name" value="Alice"]
```

- Alle Variablen sind **Strings**
- Zahlen und boolesche Werte werden ebenfalls als Zeichenfolgen gespeichert

### 7.2 Variable Erweiterung

Sie können die Variablenerweiterung innerhalb von Tag-Werten und Text verwenden.

```PlainText
[text text="${player_name} stands up"]
```

- `${...}` wird zur Laufzeit durch den Variablenwert ersetzt
- Die Erweiterung erfolgt **wenn das Tag ausgeführt wird**

```PlainText
[set name="x" value="1"]
[text text="${x}"]
[set name="x" value="2"]
[text text="${x}"]
```

In diesem Beispiel ist die Ausgabe `1` und dann `2`.

---

## 8. Makros sind Blöcke, die Sie als Einheit ausführen können

Mit Makros können Sie mehrere Tags gruppieren und gemeinsam ausführen.

```PlainText
[defmacro name="greet"]
  [text text="Hello"]
[endmacro]

[callmacro name="greet"]
```

---

## 9. Dateien wechseln (laden)

```PlainText
[load file="scene2.txt" label="start"]
```

`file=` ist obligatorisch und `label=` ist optional.

---

## 10. Was passiert am Ende einer Datei?

Bei Ausführung:

- Erreicht das Ende der Datei und
- Es passieren keine weiteren `goto`- oder `load`-Tags

Dann endet die **Szenarioausführung**.

Auf einigen Plattformen (zum Beispiel iOS oder Spielekonsolen) darf die App jedoch nicht beendet werden.

Daher wird empfohlen, dass Ihr Szenario **ausdrücklich**:

- Springt zurück zum Titel mit `goto`, oder
- Lädt eine Titelszene mit `load`

anstatt automatisch zu enden.

---

## 11. Wie dieses Dokument mit anderen Dokumenten zusammenpasst

Dieses Dokument soll begleiten:

- Tag-Referenz: erklärt, was jedes Tag bewirkt
- Dieses Dokument: erklärt, wie ein Szenario abläuft

Einzelheiten zu einzelnen Tags finden Sie in der „Tag-Referenz“.
