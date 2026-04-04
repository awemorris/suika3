NovelML Syntax Reference
========================

This document explains the basic syntax of NovelML and how it runs.

For the detailed explanation of each tag, please see [Suika3 Tag Reference](tag.md).

---

## 1. NovelML is a list of engine commands called “tags”

NovelML is a list of **tags**. A tag is a **command for the engine**.

- Each tag tells the engine to do something
- Tags run **one by one**, in the order they appear
- After a tag runs, it is "done" and the "execution position" moves on

If you write the same tag again, it will do the same thing **each time it runs**.

---

## 2. Execution goes from top to bottom

NovelML runs from the top of the file to the bottom, one tag at a time.

- Execution normally moves forward
- There is only one current **execution position**

```PlainText
[text text="Hello"]
[text text="World"]
```

In this case:

1. `Hello` is shown
2. Then `World` is shown

---

## 3. Everything must be written as tags

In NovelML, **every line must be a tag**.

- The whole scenario is written using tags
- Even text must use the `[text]` tag

```PlainText
[text text="It's a beautiful day."]
```

---

## 4. Tags that wait, and tags that do not wait

There are two main kinds of tags:

- **Tags that run and immediately move to the next tag**
- **Tags that wait** for user input or for something to finish

Common "wait' tags include:

- `text` (waits for a click)
- `click` (waits for a click)
- `wait` (waits for a specified time)
- `video` (waits until playback finishes)

These tags pause execution **without any special syntax**.

### 4.1 Tags that run in the background (asynchronously)

Some tags may **run in the background**  (asynchronously).

Typical examples:

- `anime`
- `move`

These tags:

- Create and start an animation or movement
- If `async="true"` is specified, the engine **does not wait** for it to finish and immediately continue to the next tag

Because of this, tags after an `async="true"` tag may run **while the animation is still playing**.

This lets you do things like:

- Move a background or character
- While also showing text or running other effects

---

## 5. Tags that change the flow of execution

Normally execution goes from top to bottom, but some tags **change where execution continues**.

### 5.1 Labels

```PlainText
[label start]
```

- A label marks a named position in the scenario
- Running a label does not do anything by itself

### 5.2 Jumps

```PlainText
[goto name="start"]
```

---

## 6. Conditional branching (if / elseif / else)

```PlainText
[if ...]
  ...
[elseif ...]
  ...
[else]
  ...
[endif]
```

- Only the first matching block is executed
- Blocks that are not chosen are **skipped completely**
- You can write `elseif` zero or more times
- `else` is optional

---

## 7. Variables and variable expansion

### 7.1 Setting variables

Use the `[set]` tag to set a variable.

```PlainText
[set name="player_name" value="Alice"]
```

- All variables are **strings**
- Numbers and booleans are also stored as strings

### 7.2 Variable expansion

You can use variable expansion inside tag values and text.

```PlainText
[text text="${player_name} stands up"]
```

- `${...}` is replaced with the variable value at runtime
- Expansion happens **when the tag is executed**

```PlainText
[set name="x" value="1"]
[text text="${x}"]
[set name="x" value="2"]
[text text="${x}"]
```

In this example, the output is `1` and then `2`.

---

## 8. Macros are blocks you can run as a unit

Macros let you group several tags and run them together.

```PlainText
[defmacro name="greet"]
  [text text="Hello"]
[endmacro]

[callmacro name="greet"]
```

---

## 9. Switching files (load)

```PlainText
[load file="scene2.txt" label="start"]
```

`file=` is mandatory and `label=` is optional.

---

## 10. What happens at the end of a file

If executed:

- Reaches the end of the file, and
- No more `goto` or `load` tags happen

Then the **scenario execution ends**.

However, on some platforms (for example iOS or game consoles), the app is not allowed to quit.

So it is recommended that your scenario **explicitly**:

- Jumps back to the title using `goto`, or
- Loads a title scene using `load`

instead of ending automatically.

---

## 11. How this document fits with other docs

This document is meant to go with:

- [Tag Reference](tag.md): explains what each tag does
- This document: explains how a scenario runs

For details on individual tags, see the [Tag Reference](tag.md).
