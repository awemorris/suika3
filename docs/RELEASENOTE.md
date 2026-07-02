# Suika3 26.07 LTS Series

I am pleased to be able to release the first LTS here.

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
* @Kotsuider ... reporting critical bugs

## Notes

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

Dependencies:
- Playfield Engine: v1.0.18
- NoctLang: v1.1.3
- StratoHAL: v1.0.20

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
