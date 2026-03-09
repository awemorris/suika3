Playfield Engine 設計概要
========================

## 移植レイヤ

`Playfield Engine` が Windows, macOS, Linux, iOS, Android, そして家庭用ゲーム機という、多様な環境で動作することをご存知でしょうか？

`extern/StratoHAL` ディレクトリにはプラットフォーム抽象化レイヤが含まれす。
これはプラットフォームごとに違いをきれいに吸収するものです。

### 共通コンポーネント

|File           |Description                              |
|---------------|-----------------------------------------|
|image.c        |画像処理                                 |
|stdfile.c      |標準 C ライブラリによるファイルアクセス  |
|glyph.c        |FreeType によるフォント描画              |
|wave.c         |OggVorbis デコーダ                       |

### Windows 用

|File           |Description                         |
|---------------|------------------------------------|
|winmain.c      |wWinMain()                          |
|d3drender.c    |レンダリングディスパッチ            |
|d3d12render.cc |Direct3D 12                         |
|d3d11render.cc |Direct3D 11                         |
|d3d9render.cc  |Direct3D 9                          |
|gdirender.c    |GDI rendering                       |
|dsound.c       |DirectSound audio                   |
|dsvideo.cc     |DirectShow video                    |

### macOS 用

|File           |Description                         |
|---------------|------------------------------------|
|nsmain.m       |main() (AppKit)                     |
|GameRenderer.m |Metal                               |
|aunit.c        |Audio Unit                          |

### Linux / BSD 用

|File           |Description                         |
|---------------|------------------------------------|
|x11main.c      |X11 main()                          |
|glrender.c     |OpenGL                              |
|asound.c       |ALSA                                |
|bsdsound.c     |/dev/dsp and /dev/audio             |

### iOS 用

|File           |Description                         |
|---------------|------------------------------------|
|uimain.m       |main() (UIKit)                      |
|GameRenderer.m |Metal                               |
|aunit.c        |Audio Unit                          |

### Android 用

|File              |Description                           |
|------------------|--------------------------------------|
|MainActivity.java |Android activity                      |
|ndkmain.c         |Android NDK (main)                    |
|ndkfile.c         |Android NDK (file)                    |

### SIMD

|File           |Description                         |
|---------------|------------------------------------|
|drawimage.h    |Definitions of drawing functions    |
|avx2.c         |AVX2                                |
|avx.c          |AVX                                 |
|sse42.c        |SSE4.2                              |
|sse4.c         |SSE4.1                              |
|sse3.c         |SSE3                                |
|sse2.c         |SSE2                                |
|sse.c          |SSE                                 |

## メインレイヤー

`Playfield Engine` の API を定義するメインレイヤは `src/*.c` にあります。
`src/api.c` にて C API を実装した上で、それを `src/vm.c` にて Noct でラッピングしてあります。

### タグファイル

タグつきファイルはオプションで利用可能な機能としてサポートされています。
