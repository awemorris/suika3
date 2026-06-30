BUGS
====

* QA periods
    * QA-26.07 started in 14 March 2026, will end in 30 June 2026.

---

## Video playback is not working properly after a rendering.

* Report Details
    * ID: BUG-20260314-001
    * Status: Resolved
    * Component: StratoHAL / Video / DirectShow / MediaFoundation
    * Severity: major
    * Priority: high
    * Reproducibility: always
    * First Found In: 6fda2bb1cf9155e0616043f5040f9793db3e8276
    * Fixed In: 820d539727b8b431ed55c79c44ee57969d67b10d
    * Reported Date: 15:00 14 March 2026
    * Fixed Date: 20:30 14 March 2026
    * Detection: manual test
    * Root Cause Type: window management / event handling
    * OS: All Windows
    * CPU: x86, x86_64, arm64

### Report

Cannot play video files after a rendering.
Playing a video at the very first frame seems okay.

### Analysis

StratoHAL reused the main HWND of Direct3D 12 rendering to Media
Foundation EVR playback. However, once Direct3D swapchain is created
for the HWND, it cannot be shared with EVR.

### Patch

Added a rendering child HWND and a video child HWND.

### Commits

* d0aaa9a748381fb6736154dcc432f867275201f5 Fix Media Foundation video playback
* 820d539727b8b431ed55c79c44ee57969d67b10d Fix DirectShow

---

## Video aspect ratio is wrong if DirectShow is used

* Report Details
    * ID: BUG-20260314-002
    * Status: Resolved
    * Component: StratoHAL / Video / DirectShow
    * Severity: major
    * Priority: high
    * Reproducibility: always
    * First Found In: 6fda2bb1cf9155e0616043f5040f9793db3e8276
    * Fixed In: be8e6b359f763614c7577cfa590e23972729e96f
    * Reported Date: 15:30 14 March 2026
    * Fixed Date: 21:00 14 March 2026
    * Detection: manual test
    * Root Cause Type: window management / event handling
    * OS: All Windows
    * CPU: x86

### Report

If DirectShow is used for a video playback, the aspect ratio is wrong.

### Analysis

StratoHAL lacked the code for aspect ratio adjustment.

### Patch

Added a logic to StratoHAL.

### Commits

* 18b4a766e84791154870e7592863c49367d986cf Fix aspect ratio for DirectShow
* be8e6b359f763614c7577cfa590e23972729e96f Fix DirectShow bug

---

## Video playback cannot be skipped

* Report Details
    * ID: BUG-20260314-003
    * Status: Resolved
    * Component: StratoHAL / Video / DirectShow / Media Foundation
    * Severity: major
    * Priority: high
    * Reproducibility: always
    * First Found In: 6fda2bb1cf9155e0616043f5040f9793db3e8276
    * Fixed In: 82f0e0e86e3349269bf95a809b0a061eebc5cf40
    * Reported Date: 16:00 14 March 2026
    * Fixed Date: 22:00 14 March 2026
    * Detection: manual test
    * Root Cause Type: window management / event handling
    * OS: All Windows
    * CPU: x86, x86_64, arm64

### Report

Video playback cannot be skipped by a click.

### Analysis

In StratoHAL, the video HWND didn't receive mouse events.

### Patch

Added event handlers for the video HWND in StratoHAL.

### Commits

* 82f0e0e86e3349269bf95a809b0a061eebc5cf40 Fix winmain.c to input clicks on video playback

---

## Font color is wrong

* Report Details
    * ID: BUG-20260316-001
    * Status: Resolved
    * Component: StratoHAL / image / glyph
    * Severity: high
    * Priority: high
    * Reproducibility: always
    * First Found In: 6fda2bb1cf9155e0616043f5040f9793db3e8276
    * Fixed In: 82f0e0e86e3349269bf95a809b0a061eebc5cf40
    * Reported Date: 03:00 16 March 2026
    * Fixed Date: 05:00 16 March 2026
    * Detection: manual test
    * Root Cause Type: Invalid CMake configuration
    * OS: Linux X11 (No OpenGL), Solaris 11
    * CPU: All

### Report

Font color is always wrong, the red component and the blue component is swapped.

### Analysis

In StratoHAL, we define the `HAL_USE_X11_SOFTRENDER` macro in
`CMakeLists.txt` to detect software rendering at
`platform.h`. However, that definition was defined by `PRIVATE`
scope. Therefore, Playfield Engine didn't define the same macro. As a
result, the return value of `hal_make_pixel()` was valid on StratoHAL
and invalid on Playfield Engine and Suika3.

### Patch

Fixed CMakeLists.txt of StratoHAL.

### Commits

* 9ec64623285639ba3a8831ac2b7e67f3b7a8e735 Merge upstream

---

## Pencil tag is missing

* Report Details
    * ID: BUG-20260316-001
    * Status: Resolved
    * Component: Suika3
    * Severity: high
    * Priority: high
    * Reproducibility: always
    * First Found In: b4e84027f0c42a66bc9c071232a5d1d272a73cad
    * Fixed In: 382197f510f7c9a13e1509e037761e02cd104c87
    * Reported Date: 12:00 1 April 2026
    * Fixed Date: 23:00 4 April 2026
    * Detection: document inspection
    * Root Cause Type: overlooked on porting
    * OS: All
    * CPU: All

### Report

A Suika2 command `@pencil` should be ported to `[pencil]` tag, but it's missing.

### Analysis

In the porting process from Suika2 to Suika3, that command was overlooked.

### Patch

Added the `[pencil]` tag and `Suika.drawTextOnLayer()` API.

Added/fixed:
- CMakeLists.txt
- src/api.c
- src/cmd_pencil.c

### Commits

* 382197f510f7c9a13e1509e037761e02cd104c87

---

## Mouse click position is wrong on X11

* Report Details
    * ID: BUG-20260409-001
    * Status: Resolved
    * Component: StratoHAL
    * Severity: high
    * Priority: high
    * Reproducibility: always
    * First Found In: 38ce0f3dcfea5c109b287b52a3b4bb49584015db
    * Fixed In: 3cf43369f4a71ac8eda40ff0528baa1b1bee74ba
    * Reported Date: 11:20 9 April 2026
    * Fixed Date: 11:25 9 April 2026
    * Detection: buzz testing
    * Root Cause Type: copy-and-taste misstake
    * OS: Linux, *BSD, Solaris
    * CPU: All

### Report

While mouse move position is good, mouse click position seems bad.

### Analysis

In `event_button_release()` in `x11main.c` and `x11softmain.c`,
mouse offset and scaling is bad.

### Patch

`event_button_release()` is fixed.

Fixed:
- external/PlayfieldEngine/external/StratoHAL/src/x11main.c
- external/PlayfieldEngine/external/StratoHAL/src/x11softmain.c

### Commits

- 3cf43369f4a71ac8eda40ff0528baa1b1bee74ba

---

## Sequence of a save and loads will clear background

* Report Details
    * ID: BUG-20260419-001
    * Status: Resolved
    * Component: Suika3
    * Severity: high
    * Priority: high
    * Reproducibility: always
    * First Found In: dcc84e48e887931ad43413b53a29f1ae2a544092
    * Fixed In: 3fa47574eb076724a968b1f35481ecdf8756ada3
    * Reported Date: 15:00 18 April 2026
    * Fixed Date: 00:15 19 April 2026
    * Detection: found in a testing for NVL
    * Root Cause Type: Internal API misusage
    * OS: All
    * CPU: All
    * GitHub Issue: 

### Report

After performing a sequence of saving and loading
(specifically save -> load -> load),
the background image fails to load.

### Analysis

In `s3_set_layer_image()`, `destroy_layer()` is called
internally. This is a misuse of the internal API;
`destroy_layer()` also frees the filename of the layer.

Consequently, the following sequence in `save.c` corrupts the layer's
filename:

```
s3_set_layer_file_name()
s3_set_layer_image()
```

`s3_set_layer_image()` should call `s3_destroy_image()` instead of
destroy_layer().

### Patch

`s3_set_layer_image()` has been fixed.

Files modified:
- src/stage.c

### Commits

- 3fa47574eb076724a968b1f35481ecdf8756ada3

---

## Load causes a crash in NVL mode

* Report Details
    * ID: BUG-20260419-002
    * Status: Resolved
    * Component: Suika3
    * Severity: high
    * Priority: high
    * Reproducibility: always
    * First Found In: 81ddb990151c3b20828598eb06df9aae5d1a04d3
    * Fixed In: 288e659b1e07a5374a21fdb0bc16ee4cdc229d97
    * Reported Date: 02:00 19 April 2026
    * Fixed Date: 04:20 19 April 2026
    * Detection: found in a testing for NVL
    * Root Cause Type: Lack of boundary condition
    * OS: All
    * CPU: All

### Report

Save in NVL mode, restart Suika3, then load.
This will cause a crash.

### Analysis

In `s3_append_history()`, `last_history_top` may be `-1` after a restart.
In this situation, `history[last_history_top]` may be an invalid access.

### Patch

`s3_append_history()` has been fixed.

Files modified:
- src/history.c

### Commits

- 288e659b1e07a5374a21fdb0bc16ee4cdc229d97

---

## Cannot save on Wasm

* Report Details
    * ID: BUG-20260420-001
    * Status: Resolved
    * Component: StratoHAL
    * Severity: high
    * Priority: high
    * Reproducibility: always
    * First Found In: 86a91e17752978090ddc442743d67312d54fb7303
    * Fixed In: f67d849e208413660f27201c6697ed261ca46c51
    * Reported Date: 04:16 20 April 2026
    * Fixed Date: 06:15 20 April 2026
    * Detection: found in a multiplatform testing (GitHub #7)
    * Root Cause Type: mkdir() call missing
    * OS: All
    * CPU: All

### Report

On the Wasm version, save will cause an error.

### Analysis

In `emmain.c`, `hal_make_save_directory()` is not implemented.
In addition, that function is not called.

### Patch

`hal_make_save_directory()` has been fixed.

Files modified:
- external/PlayfieldEngine/external/StratoHAL/src/emmain.c

### Commits

- f67d849e208413660f27201c6697ed261ca46c51

---

## Cannot load save data on iOS and Wasm

* Report Details
    * ID: BUG-20260420-002
    * Status: Resolved
    * Component: StratoHAL
    * Severity: high
    * Priority: high
    * Reproducibility: always
    * First Found In: 86a91e17752978090ddc442743d67312d54fb7303
    * Fixed In: f67d849e208413660f27201c6697ed261ca46c51
    * Reported Date: 05:30 20 April 2026
    * Fixed Date: 06:15 20 April 2026
    * Detection: found in testing of BUG-20260420-001
    * Root Cause Type: wrong implementation of hal_open_rfile()
    * OS: iOS, Wasm
    * CPU: All

### Report

On the Wasm version, save --> load will cause a bad behavior.

### Analysis

In `hal_open_rfile()` in `stdfile.c`,
the treatment of save files was wrong.

### Patch

`hal_open_rfile()` has been fixed.

Files modified:
- external/PlayfieldEngine/external/StratoHAL/src/stdfile.c

### Commits

- f67d849e208413660f27201c6697ed261ca46c51

---

## Cannot clear the background by file="none"

* Report Details
    * ID: BUG-20260426-001
    * Status: Resolved
    * Component: Suika3
    * Severity: high
    * Priority: high
    * Reproducibility: always
    * First Found In: 1b7e3881fc5d466226ff53a0d5f81a3a3c8e63b5
    * Fixed In: ad6347dfc1ba47a78bee2b12fa4b789ad23d8da0
    * Reported Date: 03:00 26 April 2026
    * Fixed Date: 03:15 26 April 2026
    * Detection: developer testing (@lalalll-lalalll GitHub #21)
    * Root Cause Type: missing implementation
    * OS: iOS, Wasm
    * CPU: All

### Report

When attempting to clear the current background using the following command, the engine throws an error.
```
[bg file="none"]
```

### Analysis

Analysis:
- The logic to detect a special file name "none" was missing.
- So the engine tried to open "none" as an image file.
- But "none" doesn't have an extension such as ".png"
- Therefore the engine did output an errror "Cannot determine the file type for "%s".

### Patch

`s3i_tag_bg()` was patched.

Files modified:
- src/cmd_bg.c

### Commits

- ad6347dfc1ba47a78bee2b12fa4b789ad23d8da0

---

## Thumbnail image pixel shift in save data preview

* Report Details
    * ID: BUG-20260426-002
    * Status: Resolved
    * Component: Suika3
    * Severity: high
    * Priority: high
    * Reproducibility: always
    * First Found In: 982432d12b2352bcbe0543e5b19633f59add8e3b
    * Fixed In: b531a17f11948875afea4b7a60252af8722630f2
    * Reported Date: 22:00 26 April 2026
    * Fixed Date: 05:15 27 April 2026
    * Detection: developer testing (@lalalll-lalalll GitHub #25)
    * Root Cause Type: Lack of horizontal rollout of corrections
    * OS: All
    * CPU: All

### Report

There is a visible rendering artifact in the save
thumbnails. Specifically, the rightmost vertical column of pixels is
incorrectly shifted and appears on the far left side of the image.

### Analysis

Analysis:
- In `load_basic_save_info()`, before reading the thumbnail, reading `u32: page line` was missing.
- Therefore, image pixels were 4 bytes shifted left.

### Patch

`load_basic_save_info()` was patched.

Files modified:
- src/save.c

### Commits

- b531a17f11948875afea4b7a60252af8722630f2

---

## Full-screen/windowed switching causes crash when using Direct3D 12

* Report Details
    * ID: BUG-20260427-001
    * Status: Resolved
    * Component: StratoHAL
    * Severity: high
    * Priority: high
    * Reproducibility: frequently
    * First Found In: 88649ac3a04b1982f8867ac89184a3c6e196414b
    * Fixed In: bc985e94be84b2e827f10e68a859e3ff95c530a3
    * Reported Date: 09:00 27 April 2026
    * Fixed Date: 14:40 27 April 2026
    * Detection: Author's exploratory testing
    * Root Cause Type: Multiple and complex causes; insufficient testing on newly introduced D3D12 code.
    * OS: Windows 10/11
    * CPU: x86/x86_64/arm64

### Report

Switching from full screen mode to windowed mode may cause crash.

### Analysis

Analysis:
- Cause 1: Mistake in STL usage
    - In `D3D12EndFrame()`, `std::find()` may fail to find a valid entry.
    - Passing the result of `std::find()` directly to `erase()` caused an assertion error inside the STL.
- Cause 2: Mistake in D3D12 reinitialization
    - When switching between full-screen mode and windowed mode, D3D12 is reinitialized.
    - In `ReleaseAllD3D12Objects()`, `g_freeTextureBundleList` was not cleared during D3D12 reinitialization.
    - In `UploadTextureIfNeeded()`, `img->texture` was not set to `NULL` when D3D12 was reinitialized.
    - This caused inconsistency in texture management and led to crashes.
- Cause 3: Free-after-free
    - In `D3D12NotifyImageFree()`, a texture is added to the list `g_freeTextureBundleList`.
    - Textures in `g_freeTextureBundleList` are freed later in `D3D12EndFrame()`.
    - When switching between full-screen and windowed mode, the D3D12 context is recreated and textures are bulk-freed.
    - Pointer references from `img->texture` and `TextureBundle::pTexture` intentionally remained
      even after the texture objects themselves had been bulk-freed. (It is an intended way.)
    - `D3D12NotifyImageFree()` must ignore textures created in older contexts.
    - However, the previous implementation did not store a context ID in `TextureBundle`,
      so it had no way to distinguish stale texture pointers from valid ones.
- Cause 4: Timing of freeing
    - In the previous implementation, texture COM objects were released automatically
      by the destructor of `TextureBundle`, called from `delete`.
    - However, releasing D3D12 resources requires the D3D device to still be alive,
      but the device had already been released by the time `TextureBundle` objects
      were deleted.
    - Therefore, texture COM objects must be released explicitly with `Reset()`
      before destroying the D3D device.
    - In addition, freeing `TextureBundle` instances must be deferred until the next
      `UploadTextureIfNeeded()` call, because references from `hal_image::tex` to
      `TextureBundle` must remain valid until then. The HAL currently does not keep
      a global list of `hal_image` instances.

### Patch

`d3d12render.cc` was patched.

Files modified:
- external/PlayfieldEngine/external/StratoHAL/src/d3d12render.cc

### Commits

- bc985e94be84b2e827f10e68a859e3ff95c530a3

---

## Save index is not drawn for the "press" state

* Report Details
    * ID: BUG-20260428-001
    * Status: Resolved
    * Component: Suika3
    * Severity: mid
    * Priority: high
    * Reproducibility: always
    * First Found In: f1390063b7bbcf8daeb172f09f0998aac708de45
    * Fixed In: 40d579a6a38ab761c9612a353205a7c2cba261be
    * Reported Date: 01:00 29 April 2026
    * Fixed Date: 10:20 29 April 2026
    * Detection: GitHub Issues
    * Root Cause Type: Mistake on horizontal expansion when adding "press" state
    * OS: Windows 10/11
    * CPU: x86/x86_64/arm64

### Report

> Index number disappears when Save button is in "Clicked" state

### Analysis

In `draw_save_button()` in `gui.c`, the corresponding drawing logic
was missing due to a mistake on horizontal expansion when adding
"press" state.

### Patch

`gui.c` was patched.

Files modified:
- src/gui.c

### Commits

- 40d579a6a38ab761c9612a353205a7c2cba261be

---

## Alpha channel is filled by 255 when a save slot is empty

* Report Details
    * ID: BUG-20260428-002
    * Status: Resolved
    * Component: Suika3
    * Severity: mid
    * Priority: high
    * Reproducibility: always
    * First Found In: f1390063b7bbcf8daeb172f09f0998aac708de45
    * Fixed In: 72f38ca840ee7093b60430990fcc2ea21ad7cfe3
    * Reported Date: 01:00 29 April 2026
    * Fixed Date: 10:35 29 April 2026
    * Detection: GitHub Issues
    * Root Cause Type: Lack of testing variations
    * OS: Windows 10/11
    * CPU: x86/x86_64/arm64

### Report

> Description:
> When a Save Button uses a semi-transparent image (with Alpha
> channel), and the save slot is empty (no preview thumbnail
> generated), the transparent areas of the button incorrectly display
> as a solid color block (often black or white). This prevents the
> background image/layer underneath from being visible through the
> button.
>
> Actual Behavior:
> The transparent area of the button is filled with a solid color
> block, obscuring the background layer.
>
> Expected Behavior:
>
> For empty slots, the Save Button should respect the Alpha channel and
> allow the background image to show through. The preview image should
> only cover the button when data exists, rather than being replaced by
> a solid color placeholder.

### Analysis

In `draw_save_button()` in `gui.c`, save thumbnails were drawn even if
the corresponding save slots are empty. If a save slot is empty, the
thumbnail for the slot is cleared by `RGBA=(0, 0, 0, 0)`. However,
drawing the thumbnail image to the save slot image causes letting
alpha channel be `255` due to the characteristic of the drawing
routine, `hal_draw_image()`.

### Patch

`gui.c` was patched to avoid drawing a thumbnail if a save slot is
empty.

Files modified:
- src/gui.c

### Commits

- 72f38ca840ee7093b60430990fcc2ea21ad7cfe3
