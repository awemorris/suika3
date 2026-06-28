Ray Samples
===========

## Drawing Image Pixels

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

## Sepia Color

```
func Tag_sepiaBackground(params) {
    var bgImage = Suika.getLayerImage({layer: Suika.LAYER_BG});
    var pixels = Suika.getImagePixels({image: bgImage});

    for (y in 0..720) {
        for (x in 0..1280) {
            var pix = pixels[y * 1280 + x];
			var r = (pix) & 0xff;
			var b = (pix >> 8) & 0xff;
			var g = (pix >> 16) & 0xff;
			var a = (pix >> 24) & 0xff;
            pixels[y * 1280 + x] =
                Suika.makePixel({r: Int.from((r * 0.393) + (g * 0.769) + (b * 0.189)),
                                 g: Int.from((r * 0.349) + (g * 0.686) + (b * 0.168)),
                                 b: Int.from((r * 0.272) + (g * 0.534) + (b * 0.131)),
                                 a: a});
        }
    }
    Suika.updateImagePixels({image: bgImage});

    Suika.moveToNextTag();
    return true;
}
```
