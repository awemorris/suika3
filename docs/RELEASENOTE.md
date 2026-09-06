# Suika3 26.07 LTS Series

I am pleased to be able to release the first LTS here.

June 30, 2026
Awe Morris

## Contributors

Thanks to everyone who contributed code, pull requests, bug reports,
feature requests, testing, and discussions for this release:

* @awemorris ... Lead Developer
* @tenshi0xx ... Website
* @lalalll-lalalll ... VSCode plugin, docs
* @SandwichMan5 ... QA
* @antonialoytorrens ... QA
* @2439905184 ... reporting critical bugs
* @jhq223 ... reporting critical bugs
* @Kotsuider ... reporting and fixing critical bugs

## Notes

### Suika3 26.07.12 LTS: September 7, 2026

Thank you @Kotsuider for many contribution!

Changes:
- [update] [noct] Update to Noct 2.0.2
- [fix] Global vars save/load fix #84
- [fix] Cyrillic wrap fix and s3i_cleanup_gui fix #83
- [fix] Update gui.h #82
- [fix] Fix system GUI detection during text display to prevent text skipping #81
- [fix] [BUG] Skipping script actions when opening history with the mouse wheel #80

Noct 2.0 updates:
- Added the optimizations: up to 300x faster! (when loops are auto-vectorized for SIMD)
- Added type annotations, `let`, global variables outside function, static and inline functions, etc.
- Removed `->()` notation.

```
let animal = class {
    cry: () => { }
};

let cat = extend animal {
    cry: () => {
        print("Meow");
    }
};

static func foo() {
}

static __inline func calc(a: int, b: int) : int {
    return a + b;
}
```

### Suika3 26.07.10 LTS: July 23, 2026

Changes:
- [fix] [BUG] Access violation (0xc0000005) Crash #79

Dependencies
| Component        | Version    |
|------------------|------------|
| Playfield Engine | 1.1.3      |
| StratoHAL        | 1.1.3      |
| NoctLang         | 1.1.6      |

### Suika3 26.07.9 LTS: July 15, 2026

Changes:
- [improve] Support alpha-only changes by ch tag #74
- [fix] [bug] Generate error on unquoted parameter #75
- [bug] [confirmed] [Question / Feedback] Ctrl key only skips a single message #77
- [fix] [BUG] font.select always uses base font #78

Dependencies
| Component        | Version    |
|------------------|------------|
| Playfield Engine | 1.1.2      |
| StratoHAL        | 1.1.2      |
| NoctLang         | 1.1.6      |

### Suika3 26.07.8 LTS: July 10, 2026

Changes:
- "[BUG] Choose font outline inherits its color from Choose font (Issue #73)"

Dependencies
| Component        | Version    |
|------------------|------------|
| Playfield Engine | 1.1.1      |
| StratoHAL        | 1.1.1      |
| NoctLang         | 1.1.6      |

### Suika3 26.07.7 LTS: July 8, 2026

Changes:
- Add PC98 sound (SB16 and WSS)
- Add PC98 24bpp (Cirrus)
- Prepare for Debian packaging

Dependencies
| Component        | Version    |
|------------------|------------|
| Playfield Engine | 1.1.1      |
| StratoHAL        | 1.1.1      |
| NoctLang         | 1.1.6      |

### Suika3 26.07.6 LTS: July 5, 2026

Changes:
- Reduce memory usage (minimum 64MB -> minumum 16MB)
- Lower CPU usage for audio decoding (libvorbis --> tremor)

Dependencies
| Component        | Version    |
|------------------|------------|
| Playfield Engine | 1.1.0      |
| StratoHAL        | 1.1.0      |
| NoctLang         | 1.1.5      |

### Suika3 26.07.6 LTS: July 3, 2026

Changes:
- [fix] Fix degrade of Playfield Engine

Dependencies
| Component        | Version    |
|------------------|------------|
| Playfield Engine | 1.0.22     |
| NoctLang         | 1.1.5      |
| StratoHAL        | 1.0.24     |

### Suika3 26.07.5 LTS: July 3, 2026

Changes:
- [fix] [wasm] Fix a bug where the Wasm version cannot load assets.arc

Dependencies
| Component        | Version    |
|------------------|------------|
| Playfield Engine | 1.0.21     |
| NoctLang         | 1.1.5      |
| StratoHAL        | 1.0.24     |

### Suika3 26.07.4 LTS: July 2, 2026

Changes:
- CLI
  - New: Add a feature to specify a script file to load
    - For example, type in the console:
    ```
    suika3-cli.exe script.ray
    ```
  - New: Add File.* API in NoctLang
    - For example, type in the console:
    ```
    func main(arg) {
        var file = File.open("testfile", "r");
        var data = File.read(file, 100);
        for (i in 0..100)
            print(data[i]);
    }
    ```
  - Change: Change the file name "suika3-console.exe" to "suika3-cli.exe"
  - These features are only available for:
    - Windows: suika3-cli.exe
    - macOS: CLI version
    - Linux
    - PC98
    - PC/AT

Dependencies
| Component        | Version    |
|------------------|------------|
| Playfield Engine | 1.0.20     |
| NoctLang         | 1.1.5      |
| StratoHAL        | 1.0.23     |

### Suika3 26.07.3 LTS: July 1, 2026

Changes:
- Fix a bug where suika3-debug.exe is not a console app.
- Change the file name: suika3-debug.exe --> suika3-console.exe
- Add a feature to write PNG files (Suika.writeImage)
- Add a feature to use Suika3 as a CLI scripting runtime
  - This feature is only enabled in:
    - Windows: suika3-console.exe
    - macOS: CLI version
    - Linux
    - PC98
    - PC/AT
  - To use this, define "func main(arg)" in `main.ray`.
  - If "main()" exists, it will be executed, and the game won't start.
- Dependency Update
  - PlayfieldEngine: 1.0.17 --> 1.0.18
  - NoctLang: 1.1.3 --> 1.1.4
  - StratoHAL: 1.0.20 --> 1.0.22

### Suika3 26.07.2 LTS: June 30, 2026

Changes:
- Fix a bug of the scripting core.
  - PlayfieldEngine 1.0.16 --> 1.0.17
  - NoctLang 1.1.2 --> 1.1.3
  - Issue (https://github.com/awemorris/NoctLang/issues/3)

Dependencies:
- Playfield Engine: v1.0.17
- NoctLang: v1.1.3
- StratoHAL: v1.0.20

### Suika3 26.07.1 LTS: June 29, 2026

Changes:
- Add a tutorial document.
- Update translations.

Dependencies:
- Playfield Engine: v1.0.16
- NoctLang: v1.1.2
- StratoHAL: v1.0.20

### Suika3 26.07.0 LTS: June 29, 2026

First release.
