#!/bin/sh

mkdir -p ../../locale/en/LC_MESSAGES
mkdir -p ../../locale/es/LC_MESSAGES
mkdir -p ../../locale/fr/LC_MESSAGES
mkdir -p ../../locale/de/LC_MESSAGES
mkdir -p ../../locale/it/LC_MESSAGES
mkdir -p ../../locale/ru/LC_MESSAGES
mkdir -p ../../locale/el/LC_MESSAGES
mkdir -p ../../locale/zh_CN/LC_MESSAGES
mkdir -p ../../locale/zh_TW/LC_MESSAGES
mkdir -p ../../locale/ja/LC_MESSAGES

msgfmt po_files/en.po -o ../../locale/en/LC_MESSAGES/libnoct.mo
msgfmt po_files/es.po -o ../../locale/es/LC_MESSAGES/libnoct.mo
msgfmt po_files/fr.po -o ../../locale/fr/LC_MESSAGES/libnoct.mo
msgfmt po_files/de.po -o ../../locale/de/LC_MESSAGES/libnoct.mo
msgfmt po_files/it.po -o ../../locale/it/LC_MESSAGES/libnoct.mo
msgfmt po_files/ru.po -o ../../locale/ru/LC_MESSAGES/libnoct.mo
msgfmt po_files/el.po -o ../../locale/el/LC_MESSAGES/libnoct.mo
msgfmt po_files/zh.po -o ../../locale/zh_CN/LC_MESSAGES/libnoct.mo
msgfmt po_files/tw.po -o ../../locale/zh_TW/LC_MESSAGES/libnoct.mo
msgfmt po_files/ja.po -o ../../locale/ja/LC_MESSAGES/libnoct.mo
