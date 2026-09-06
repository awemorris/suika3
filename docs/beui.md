BeUI Non-Standard API
=====================

`BeUI.*` is an optional, small graphical environment for scripts.
It can be used on PC-98 to Linux.

---

## Lifecycle

### BeUI.init()

Opens the display and returns 1 on success, 0 when no display is
available. Calling it again while open succeeds and changes nothing.
The default bits per pixel is 8 on PC-98.

```
if (!BeUI.init()) {
    print("no display");
    return 1;
}
```

### BeUI.initWithHint(bitsPerPixel)

Opens the display while hinting that 8 or 24 bits per pixel is
preferred.  The hint is not a requirement.

### BeUI.close()

Closes the display and restores whatever mode the host was in. Held
keys are drained so they do not leak to the caller.

### BeUI.isOpen()

Returns 1 while the display is open.

### BeUI.getWidth() / BeUI.getHeight()

The display size in pixels. Only valid while open.

### BeUI.poll()

Services the host and returns 1 while the display is alive, 0 once the
user has closed the window. This is the main loop condition. A target
that owns the whole machine, such as a boot environment, never returns
0.

```
BeUI.init();
while (BeUI.poll()) {
    BeUI.fill(0, 0, BeUI.getWidth(), BeUI.getHeight(), 0);
    BeUI.drawText("Hello", 10, 10, 16777215, 0);
    BeUI.flush();
}
BeUI.close();
```

### BeUI.flush()

Presents what has been drawn. Backends that draw straight to the panel
implement it as a no-op, so it is always safe to call.

---

## Drawing

Colors are `0x00RRGGBB`. Backends with fewer colors quantize; a script
never needs to know the panel depth.

### BeUI.fill(x, y, width, height, color)

Fills a rectangle.

### BeUI.line(x0, y0, x1, y1, color)

Draws a line.

### BeUI.patternFill(x, y, width, height, color, pattern)

Fills a rectangle through an 8x8 dither pattern given as a 64-bit
integer, one bit per pixel, row major.

### BeUI.drawText(text, x, y, foreground, background)

Draws UTF-8 text. `\n` starts a new line 16 pixels down.

### BeUI.textWidth(text) / BeUI.textHeight(text)

Measures text without drawing it.

---

## Images

Images are decoded from uncompressed Windows BMP at 1, 4, 8, or 24
bits per pixel. BMP is used rather than PNG so a freestanding host
needs no DEFLATE implementation.

### BeUI.loadImage(bytes)

Decodes a byte array and returns an image handle. Reading the file is
the caller's job, which keeps BeUI independent of any filesystem and
makes the call behave identically on every host.

```
var file = File.open("logo.bmp", "rb");
var image = BeUI.loadImage(File.read(file, FileUtil.getFileSize("logo.bmp")));
File.close(file);
BeUI.drawImage(image, 100, 50);
BeUI.destroyImage(image);
```

### BeUI.drawImage(image, x, y)

Draws a loaded image.

### BeUI.getImageWidth(image) / BeUI.getImageHeight(image)

Returns the decoded image size in pixels.

### BeUI.drawImageRegion(image, sourceX, sourceY, width, height, x, y)

Draws a rectangular part of a loaded image. The source rectangle and
the destination rectangle must both fit completely inside the image
and display, respectively. This is useful for scrolling a picture
larger than the display.

### BeUI.drawImagePattern(image, x, y, pattern)

Draws a loaded image through an 8x8 dither pattern, as `patternFill`.

### BeUI.destroyImage(image)

Releases an image. Any image left loaded is released when the VM shuts
down.

---

## Input

### BeUI.isKeyDown(key)

Returns 1 while a key is held. Keys the target cannot sense read as
released, so a script never blocks on hardware that does not
exist. Key codes come from the `Key` dictionary:

```
Key.Escape  Key.Enter   Key.Tab      Key.Backspace
Key.Delete  Key.Insert  Key.Space    Key.Shift
Key.Up      Key.Down    Key.Left     Key.Right
Key.Home    Key.End     Key.PageUp   Key.PageDown
Key.F1 ... Key.F10
```

Lowercase ASCII letters and digits are their own character codes, so
`String.charCodeAt` names them.

```
var keyA = String.charCodeAt("a", 0);

while (BeUI.poll()) {
    if (BeUI.isKeyDown(Key.Escape) == 1)
        break;
    if (BeUI.isKeyDown(keyA) == 1)
        x = x - 1;
}
```

### BeUI.getPointerX() / BeUI.getPointerY()

The pointer position in absolute display coordinates, clamped to the
display. Hardware that reports motion deltas, such as the PC-98 bus
mouse, is integrated inside the backend, so scripts see one coordinate
space everywhere. The pointer starts centred.

### BeUI.getPointerButtons()

A bitmask of the buttons currently held, from the `Button` dictionary:
`Button.Left`, `Button.Right`, `Button.Middle`.

```
if ((BeUI.getPointerButtons() & Button.Left) != 0)
    select(BeUI.getPointerX(), BeUI.getPointerY());
```

---

## Time

### BeUI.getMilliseconds()

A monotonic millisecond counter. On targets whose clock is a polled
hardware counter it only advances while the script keeps calling into
BeUI, `poll` and `sleep` both count.

### BeUI.sleep(milliseconds)

Waits, servicing the backends throughout. A window closed during the
wait ends it early, and the script sees the close on its next
`BeUI.poll()`.

```
var previous = BeUI.getMilliseconds();
while (BeUI.poll()) {
    var now = BeUI.getMilliseconds();
    advance(now - previous);
    previous = now;
    BeUI.flush();
    BeUI.sleep(16);
}
```
