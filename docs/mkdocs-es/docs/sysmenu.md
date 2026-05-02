System Menu
===========

## SysBtn

Suika3 features a hamburger menu button, typically located in the
top-left corner of the screen. This button is internally referred to
as the `System Button (SysBtn)`.

To ensure compliance with App Store guidelines, Suika3 avoids placing
small, PC-style buttons around the message window, adopting a
mobile-first approach instead. The SysBtn consists of two images and
associated animations. For a seamless user experience, the button is
responsive: it appears upon mouse movement or touch, and automatically
hides after a few seconds of inactivity. While this behavior is
hard-coded for store compliance, the SysBtn can be completely
disabled—for instance, for demos or Kiosk mode—by setting
`sysbtn.enable=false` in the config.

While the absence of buttons around the familiar message window may
feel unexpected at first, we hope you'll understand that this
evolution is essential for adapting visual novels to modern mobile
platforms.

Please see also:
- `config.ini` (search for `sysbtn`)
- `system/sysbtn/` (in the sample game)

## SysMenu

Clicking the SysBtn triggers a GUI known as the `System Menu
(SysMenu)`. The SysMenu is fully customizable and can be configured to
include essential functions such as Save, Load, Auto Mode, Skip Mode,
History, and Config.

Please see also:
- `system/sysmenu/` (in the sample game)

## Save/Load Screens

Save and load screens are fully customizable via GUI files.

Please see also:
- `system/save/` (in the sample game)
- `system/load/` (in the sample game)

## Auto Mode and Skip Mode Buttons

The Auto and Skip buttons use specialized GUI button types. These
buttons trigger their respective modes—Auto Mode or Skip Mode—when
clicked.

Please see also:
- `system/sysmenu/` (in the sample game)

## History Screen

The History screen is fully customizable via a GUI file.

Please see also:
- `system/history/` (in the sample game)

## Config Screen

The Config screen is fully customizable via a GUI file.

The config screen in the sample game includes:
- BGM, Sound Effect, and Voice Volume Sliders
- Language Switch (EN/JP)
- Text and Auto Mode Speed Adjustments
- Text Speed Preview

Additionally, you can implement:
- Master Volume
- Per-Character Volume
- Language Switch (for all supported languages)
- Custom Buttons

Please see also:
- `system/config/` (in the sample game)
