Ray Samples
===========

## Drawing Images

```
func Tag_custom1(params) {
    // Get the image of the background (bg) layer.
    var bgImage = Suika.getLayerImage({layer: Suika.LAYER_BG});

    // Get the pixel buffer of the image. (UINT32)
    var pixels = Suika.getImagePixels({image: bgImage});

    // Draw gradations.
    for (y in 0..720) {
        for (x in 0..1280) {
            pixels[y * 1280 + x] = makePixel(x & 0xff, 0, 0, 0xff);
	}
    }

    // Upload the pixels to the GPU.
    Suika.updateImagePixels({image: bgImage});

    // Move to the next tag.
    Suika.moveToNextTag();

    // Succeeded.
    return true;
}

func makePixel(r, g, b, a) {
    return (r & 0xff) |
           ((g & 0xff) << 8) |
	   ((b & 0xff) << 16) |
	   ((a & 0xff) << 24);
}
```
