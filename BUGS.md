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
