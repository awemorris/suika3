In Bearbeitung

## Spieleinstellungen

| Name | Typ | Beschreibung |
|----------------------------|------------|---------------------------------------------------------------|
| game.title.en | Zeichenfolge | Spieltitel auf Englisch. |
| game.novel | Boolescher Wert | Aktivieren Sie den Romanmodus. |
| game.locale | Zeichenfolge | Erzwingen Sie die Einstellung der Sprache über die Systemgebietsschemaeinstellung. |

### game.title.en

### game.novel

### game.local

Spracheinstellung.

- „“: Systemeinstellung verwenden
- „ja“: Korrektur auf Japanisch
- „en“: Korrektur auf Englisch

--

## Schriftartdateieinstellungen

# Schriftart 1
#  - Standardschriftart
#  – Sie können diese Schriftart auswählen, indem Sie \f{1} zu einer Nachricht hinzufügen
font.ttf1=system/font/rounded-l-mplus-1c-bold.ttf

# Schriftart 2
#  - Sie können diese Schriftart auswählen, indem Sie \f{2} zu einer Nachricht hinzufügen
#font.ttf2=

# Schriftart 3
#  - Sie können diese Schriftart auswählen, indem Sie \f{3} zu einer Nachricht hinzufügen
#font.ttf3=

# Schriftart 4
#  - Sie können diese Schriftart auswählen, indem Sie \f{4} zu einer Nachricht hinzufügen
#font.ttf4=


############################################################
## Einstellungen für das Nachrichtenfeld

#
# Bild des Nachrichtenfelds
#

msgbox.image=system/message/msgbox.png

#
# Animation
#

msgbox.anime.hide=system/message/msgbox-hide.anime msgbox.anime.show=system/message/msgbox-show.anime

#
# Position des Nachrichtenfelds
#

msgbox.x=0 msgbox.y=520

#
# Ränder für Nachrichtenfeldtext in Pixel
#

msgbox.margin.left=200 msgbox.margin.top=20 msgbox.margin.right=200 msgbox.margin.bottom=30

#
# Zeilenrand für Meldungsfeldtext in Pixel
# (einschließlich Zeichenhöhe)
#

msgbox.margin.line=40

#
# Zeichenabstand für den Text im Meldungsfeld in Pixel
#

msgbox.margin.char=0

#
# Schriftartenauswahl für den Nachrichtenfeldtext (1,2,3,4)
#

msgbox.font.select=1

#
# Schriftgröße des Nachrichtenfeldtextes
#

msgbox.font.size=36

#
# Schriftfarbe des Nachrichtenfeldtextes
#

msgbox.font.r=255 msgbox.font.g=255 msgbox.font.b=255

#
# Schriftumrissbreite und Farbe des Texts des Meldungsfelds
#

msgbox.font.outline.width=0 msgbox.font.outline.r=0 msgbox.font.outline.g=0 msgbox.font.outline.b=0

#
# Ruby-Größe des Texts des Meldungsfelds
#

msgbox.font.ruby=10

#
# Aktivieren Sie das vertikale Schreiben des Textes des Meldungsfelds
#

msgbox.font.tategaki=false

#
# Aktivieren Sie die Hintergrundfüllung für Zeichen
#

msgbox.fill.enable=false msgbox.fill.r=255 msgbox.fill.g=255 msgbox.fill.b=255

#
# Aktivieren Sie die Dimmung für die vorherigen Absätze des Texts des Meldungsfelds
#

msgbox.dim.enable=false

#
# Dimmende Farbe des Texts des Meldungsfelds
#

msgbox.dim.r=0 msgbox.dim.g=0 msgbox.dim.b=0

#
# Dimmen der Umrissbreite und Farbe des Texts des Meldungsfelds
# msgbox.dim.outline.width=0 msgbox.dim.outline.r=0 msgbox.dim.outline.g=0 msgbox.dim.outline.b=0

#
# Aktivieren Sie die Änderung der Schriftfarbe der angezeigten Nachrichten im Nachrichtenfeld
#

msgbox.seen.enable=false

#
# Schriftfarbe der angezeigten Nachrichten des Nachrichtenfelds
#

msgbox.seen.r=0 msgbox.seen.g=0 msgbox.seen.b=0

#
# Schriftumrissbreite und -farbe der angezeigten Meldungen im Meldungsfeld
#

msgbox.seen.outline.width=0 msgbox.seen.outline.r=0 msgbox.seen.outline.g=0 msgbox.seen.outline.b=0

#
# Aktivieren Sie das Überspringen für nicht gesehene Nachrichten
#

msgbox.skip_unseen=false

############################################################
## Namensfeldeinstellungen

#
# Aktivieren Sie das Namensfeld
#  - Deaktivieren Sie das Namensfeld, wenn Sie den Romanstil im Vollbildmodus benötigen
#

namebox.enable=wahr

#
# Bild des Namensfelds
#

namebox.image=system/message/namebox.png

#
# Animation
#

namebox.anime.hide=system/message/namebox-hide.anime namebox.anime.show=system/message/namebox-show.anime

#
# Position des Namensfeldes
#

namebox.x=80 namebox.y=450

#
# Ränder des Texts des Namensfelds in Pixel
#

namebox.margin.top=5 namebox.margin.left=0

#
# Aktivieren Sie die Zentrierung des Textes des Namensfelds
#

namebox.centering=wahr

#
# Schriftartauswahl des Textes des Namensfeldes (1,2,3,4)
#

namebox.font.select=1

#
# Schriftgröße des Textes des Namensfeldes
#

namebox.font.size=36

#
# Schriftfarbe des Textes des Namensfeldes
#

namebox.font.r=255 namebox.font.g=255 namebox.font.b=255

#
# Schriftumrissbreite und Farbe des Textes des Namensfeldes
#

namebox.font.outline.width=0 namebox.font.outline.r=255 namebox.font.outline.g=255 namebox.font.outline.b=255

#
# Ruby-Größe des Textes des Namensfelds
#

namebox.font.ruby=10

#
# Aktivieren Sie das vertikale Schreiben des Textes des Namensfelds
#

namebox.font.tategaki=false


############################################################
## Klicken Sie auf Animationseinstellungen

#
# Position der Klickanimation
#

click.x=1060 click.y=660

#
# Intervall der Klickanimation
#

click.interval=1.0

#
# Bilder der Klick-Animation
#

click.image1=system/message/click1.png click.image2=system/message/click2.png #click.image3= #click.image4= #click.image5= #click.image6= #click.image7= #click.image8= #click.image9= #click.image10= #click.image11= #click.image12= #click.image13= #click.image14= #click.image15= #click.image16=

#
# Aktivieren Sie die automatische Verschiebung der Klickanimation
#

click.move=false


############################################################
## Wählen Sie Box-Einstellungen

#
# Schriftartauswahl des Textes der Auswahlfelder
#

choose.font.select=1

#
# Schriftgröße des Textes der Auswahlfelder
#

choose.font.size=36

#
# Schriftfarbe des Textes des nicht spitzen Auswahlfelds
#

choose.font.idle.r=255 choose.font.idle.g=255 choose.font.idle.b=255

#
# Breite des Schriftumrisses und Farbe des Textes des nicht spitz zulaufenden Auswahlfelds
#

choose.font.idle.outline.width=0 choose.font.idle.outline.r=255 choose.font.idle.outline.g=255 choose.font.idle.outline.b=255

#
# Schriftfarbe des Textes des spitzen Auswahlfelds
#

choose.font.hover.r=255 choose.font.hover.g=0 choose.font.hover.b=0

#
# Breite des Schriftumrisses und Farbe des Texts des spitzen Auswahlfelds
#

choose.font.hover.outline.width=0 choose.font.hover.outline.r=255 choose.font.hover.outline.g=255 choose.font.hover.outline.b=255

#
# Ruby-Größe des Textes der Auswahlfelder
#

choose.font.ruby=10

#
# Aktivieren Sie das vertikale Schreiben des Textes der Auswahlfelder
#

choose.font.tategaki=false

#
# Soundeffekt, wenn das spitze Auswahlfeld geändert wird
#

choose.change_se=system/choose/button.ogg

#
# Soundeffekt, wenn ein Auswahlfeld ausgewählt ist
#

choose.click_se=system/choose/button.ogg

#
# Einstellungen für das Auswahlfeld 1
#

choose.box1.idle=system/choose/idle.png choose.box1.hover=system/choose/hover.png choose.box1.x=200 choose.box1.y=130 choose.box1.margin.top=15 choose.box1.idle_anime=system/choose/idle.anime choose.box1.hover_anime=system/choose/hover.anime

#
# Einstellungen für das Auswahlfeld 2
#

choose.box2.idle=system/choose/idle.png choose.box2.hover=system/choose/hover.png choose.box2.x=200 choose.box2.y=220 choose.box2.margin.top=15 choose.box2.idle_anime=system/choose/idle.anime choose.box2.hover_anime=system/choose/hover.anime

#
# Einstellungen für das Auswahlfeld 3
#

choose.box3.idle=system/choose/idle.png choose.box3.hover=system/choose/hover.png choose.box3.x=200 choose.box3.y=310 choose.box3.margin.top=15 choose.box3.idle_anime=system/choose/idle.anime choose.box3.hover_anime=system/choose/hover.anime

#
# Einstellungen für das Auswahlfeld 4
#

choose.box4.idle=system/choose/idle.png choose.box4.hover=system/choose/hover.png choose.box4.x=200 choose.box4.y=400 choose.box4.margin.top=15 choose.box4.idle_anime=system/choose/idle.anime choose.box4.hover_anime=system/choose/hover.anime

#
# Einstellungen für das Auswahlfeld 5
#

choose.box5.idle=system/choose/idle.png choose.box5.hover=system/choose/hover.png choose.box5.x=200 choose.box5.y=490 choose.box5.margin.top=15 choose.box5.idle_anime=system/choose/idle.anime choose.box5.hover_anime=system/choose/hover.anime

#
# Einstellungen für das Auswahlfeld 6
#

choose.box6.idle=system/choose/idle.png choose.box6.hover=system/choose/hover.png choose.box6.x=200 choose.box6.y=580 choose.box6.margin.top=15 choose.box6.idle_anime=system/choose/idle.anime choose.box6.hover_anime=system/choose/hover.anime

#
# Einstellungen für das Auswahlfeld 7
#

choose.box7.idle=system/choose/idle.png choose.box7.hover=system/choose/hover.png choose.box7.x=200 choose.box7.y=670 choose.box7.margin.top=15 choose.box7.idle_anime=system/choose/idle.anime choose.box7.hover_anime=system/choose/hover.anime

#
# Einstellungen für das Auswahlfeld 8
#

choose.box8.idle=system/choose/idle.png choose.box8.hover=system/choose/hover.png choose.box8.x=200 choose.box8.y=760 choose.box8.margin.top=15 choose.box8.idle_anime=system/choose/idle.anime choose.box8.hover_anime=system/choose/hover.anime


############################################################
## Dateneinstellungen speichern
############################################################

#
# Größe der gespeicherten Miniaturansicht
#

save.thumb.width=213 save.thumb.height=120

#
# Bild „Neu“ markieren
#

#save.new_image=system/save/new.png


############################################################
## Systemtasteneinstellungen

#
# Aktivieren Sie die Systemtaste
#

sysbtn.enable=wahr

#
# Bilder der Systemtaste
#

sysbtn.idle=system/sysbtn/sysbtn-idle.png sysbtn.hover=system/sysbtn/sysbtn-hover.png

#
# Anime
#

# Versteckt.
sysbtn.anime.out=system/sysbtn/anime-out.anime

# Erscheint.
sysbtn.anime.fadein=system/sysbtn/anime-fadein.anime

# Erscheint, aber nicht spitz.
sysbtn.anime.appear=system/sysbtn/anime-appear.anime

# Schwebte.
sysbtn.anime.hover=system/sysbtn/anime-hover.anime

# Verschwinden.
sysbtn.anime.fadeout=system/sysbtn/anime-fadeout.anime

#
# Position der Systemtaste
#

sysbtn.x=1183 sysbtn.y=48 sysbtn.width=100 sysbtn.height=100

#
# Soundeffekte der Systemtaste
#

#sysbtn.enter_se=se/click.ogg #sysbtn.leave_se=se/click.ogg #sysbtn.click_se=se/click.ogg


############################################################
## Einstellungen für den automatischen Modus

#
# Bild des Automodus-Banners
#

automode.image=system/message/auto.png

#
# Animation
#

automode.anime.hide=system/message/auto-hide.anime automode.anime.show=system/message/auto-show.anime

#
# Position des Automodus-Banners
#

automode.x=0 automode.y=126

#
# Soundeffekt beim Aufrufen des Automatikmodus
#

#automode.enter_se=

#
# Soundeffekt beim Verlassen des Automodus
#

#automode.leave_se=


############################################################
## Einstellungen für den Überspringmodus

#
# Bild des Sprungmodus-Banners
#

skipmode.image=system/message/skip.png

#
# Animation
#

skipmode.anime.hide=system/message/skip-hide.anime skipmode.anime.show=system/message/skip-show.anime

#
# Position des Automodus-Banners
#

skipmode.x=0 skipmode.y=126

#
# Soundeffekt beim Aufrufen des Skip-Modus
#

#skipmode.enter_se=

#
# Soundeffekt beim Verlassen des Automodus
#

#skipmode.leave_se=


############################################################
## GUI-Einstellungen

#
# Schriftartenauswahl für den Slot-Indextext der save/load GUI-Elemente
# gui.save.index.font.select=1 gui.save.index.font.size=30 gui.save.index.font.r=255 gui.save.index.font.g=200 gui.save.index.font.b=200 gui.save.index.font.outline.width=0 gui.save.index.font.outline.r=0 gui.save.index.font.outline.g=0 gui.save.index.font.outline.b=0 gui.save.index.font.ruby=10 gui.save.index.font.tategaki=false gui.save.index.margin.char=3

#
# Schriftartauswahl für den Datumstext der save/load GUI-Elemente
# gui.save.date.font.select=1 gui.save.date.font.size=30 gui.save.date.font.r=255 gui.save.date.font.g=255 gui.save.date.font.b=255 gui.save.date.font.outline.width=0 gui.save.date.font.outline.r=0 gui.save.date.font.outline.g=0 gui.save.date.font.outline.b=0 gui.save.date.font.ruby=10 gui.save.date.font.tategaki=false gui.save.date.margin.char=5

#
# Schriftartenauswahl für den Kapiteltext der save/load GUI-Elemente
# gui.save.chapter.font.select=1 gui.save.chapter.font.size=32 gui.save.chapter.font.r=255 gui.save.chapter.font.g=255 gui.save.chapter.font.b=255 gui.save.chapter.font.outline.width=2 gui.save.chapter.font.outline.r=32 gui.save.chapter.font.outline.g=32 gui.save.chapter.font.outline.b=32 gui.save.chapter.font.ruby=10 gui.save.chapter.font.tategaki=false gui.save.chapter.margin.char=5

#
# Schriftartenauswahl für den Nachrichtentext der save/load GUI-Elemente
# gui.save.msg.font.select=1 gui.save.msg.font.size=22 gui.save.msg.font.r=255 gui.save.msg.font.g=255 gui.save.msg.font.b=255 gui.save.msg.font.outline.width=1 gui.save.msg.font.outline.r=32 gui.save.msg.font.outline.g=32 gui.save.msg.font.outline.b=32 gui.save.msg.font.ruby=10 gui.save.msg.font.tategaki=false gui.save.msg.margin.line=30 gui.save.msg.margin.char=0 gui.save.msg.multiline=wahr

#
# Auswahl der Schriftart für den Namen und den Text der Verlaufs-GUI-Elemente
#

gui.history.name.font.select=1 gui.history.name.font.size=34 gui.history.name.font.r=255 gui.history.name.font.g=245 gui.history.name.font.b=245 gui.history.name.font.outline.width=1 gui.history.name.font.outline.r=32 gui.history.name.font.outline.g=32 gui.history.name.font.outline.b=32 gui.history.name.font.ruby=10 gui.history.name.font.tategaki=false gui.history.name.margin.line=40 gui.history.name.margin.char=0

gui.history.text.font.select=1 gui.history.text.font.size=32 gui.history.text.font.r=255 gui.history.text.font.g=255 gui.history.text.font.b=255 gui.history.text.font.outline.width=1 gui.history.text.font.outline.r=32 gui.history.text.font.outline.g=32 gui.history.text.font.outline.b=32 gui.history.text.font.ruby=10 gui.history.text.font.tategaki=false gui.history.text.margin.line=40 gui.history.text.margin.char=0

#
# Zeilenzitat des Textes der Verlaufs-GUI-Elemente
#

gui.history.quote.name_separator=\n gui.history.quote.start=" gui.history.quote.end="

#
# Letztes Verlaufselement ausblenden
#

gui.history.hide_last=false

#
# Schriftartauswahl des Textes des Textvorschau-GUI-Elements (1,2,3,4)
#

gui.preview.font.select=1

#
# Schriftgröße des Textes des Textvorschau-GUI-Elements
#

gui.preview.font.size=36

#
# Schriftfarbe des Textes des Textvorschau-GUI-Elements
#

gui.preview.font.r=255 gui.preview.font.g=255 gui.preview.font.b=255

#
# Schriftumrissbreite und Farbe des Texts des Textvorschau-GUI-Elements
#

gui.preview.font.outline.width=0 gui.preview.font.outline.r=255 gui.preview.font.outline.g=255 gui.preview.font.outline.b=255

#
# Ruby-Größe des Textes des Textvorschau-GUI-Elements
#

gui.preview.font.ruby=10

#
# Aktivieren Sie das vertikale Schreiben des Textes des Textvorschau-GUI-Elements
#

gui.preview.font.tategaki=false


############################################################
## Toneinstellungen

#
# Anfangswert der BGM-Track-Lautstärke
#

sound.vol.bgm=1.0

#
# Anfangswert der Lautstärke der Sprachspur
#

sound.vol.voice=1.0

#
# Anfangswert der SE-Track-Lautstärke
#

sound.vol.se=1.0

#
# Anfangswert der Volumina pro Zeichen
#

sound.vol.per_character=1.0


############################################################
## Charaktereinstellungen

#
# Charakternamen
#  - für Namensübersetzung, Lippensynchronisation, Autofokus usw.
#

character.name1=Midori character.name1.en=Midori character.name1.zh-cn=美登利 character.name1.zh-tw=美登利 character.name1.ja=みどり

character.name2=Xiaoling character.name2.en=Xiaoling character.name2.zh-cn=小玲 character.name2.zh-tw=小玲 character.name2.ja=シャオリン

#character.name3= #character.name4= #character.name5= #character.name6= #character.name7= #character.name8= #character.name9= #character.name10= #character.name11= #character.name12= #character.name13= #character.name14= #character.name15= #character.name16= #character.name17= #character.name18= #character.name19= #character.name20= #character.name21= #character.name22= #character.name23= #character.name24= #character.name25= #character.name26= #character.name27= #character.name28= #character.name29= #character.name30= #character.name31= #character.name32=

#
# Unterordner für Charakterbilder (für Lippensynchronisation und automatischen Fokus)
#

character.folder1=ch/midori/
character.folder2=ch/xiaoling/
#character.folder3=
#character.folder4=
#character.folder5=
#character.folder6=
#character.folder7=
#character.folder8=
#character.folder9=
#character.folder10=
#character.folder11=
#character.folder12=
#character.folder13=
#character.folder14=
#character.folder15=
#character.folder16=
#character.folder17=
#character.folder18=
#character.folder19=
#character.folder20=
#character.folder21=
#character.folder22=
#character.folder23=
#character.folder24=
#character.folder25=
#character.folder26=
#character.folder27=
#character.folder28=
#character.folder29=
#character.folder30=
#character.folder31=
#character.folder32=

#
# Eye blinking interval in seconds
#

character.eyeblink.interval=4.0

#
# Länge des Augenblinzelrahmens in Sekunden
#

character.eyeblink.frame=0.05

#
# Länge des Lippensynchronisationsrahmens in Sekunden
#

character.lipsync.frame=0.04

#
# Lippensynchronisationsrahmen pro Zeichen
#

character.lipsync.chars=14

############################################################
## Autofokus-Einstellungen
##
## Hinweis: Funktioniert noch nicht auf RC1

#
# Automatische Fokussierung auf Sprecher / Unfokussierung von Nicht-Sprechern auf [Text] mit Namen
# #autofocus.on_text_name=true

#
# Fokussierung aller Zeichen auf [Text] ohne Namen automatisch aufheben
# #autofocus.on_text_no_name=true

#
# Automatische Unfokussierung von Nicht-Sprechern auf [ch]
# #autofocus.on_ch=true

#
# Fokussierung aller Charaktere auf [Auswahl] automatisch aufheben
# #autofocus.on_choose=true


############################################################
## Bühneneinstellungen

#
# Zeichenränder in Pixel
#

stage.ch_margin.bottom=0 stage.ch_margin.left=0 stage.ch_margin.right=0


############################################################
## Kira Kira-Effekteinstellungen

#
# Aktivieren Sie den Kira-Kira-Effekt (Klickeffektanimation).
#

kirakira.enable=false

#
# Aktivieren Sie „Add-Blend“ für den Kira-Kira-Effekt
#

kirakira.add_blend=false

#
# Bildlänge für den Kira-Kira-Effekt in Sekunden
#

kirakira.frame=0.333

#
# Bilder für Kira Kira-Effekt
#

#kirakira.image1=kira1.png #kirakira.image2=kira2.png #kirakira.image3=kira3.png #kirakira.image4=kira4.png #kirakira.image5= #kirakira.image6= #kirakira.image7= #kirakira.image8= #kirakira.image9= #kirakira.image10= #kirakira.image11= #kirakira.image12= #kirakira.image13= #kirakira.image14= #kirakira.image15= #kirakira.image16=


############################################################
## Emoji

#
# Emojis (1 bis 32)
#

emoji.name1=Herz emoji.image1=system/emoji/heart.png

emoji.name2=Schweiß emoji.image2=system/emoji/sweat.png


############################################################
## Text-zu-Sprache

#
# Aktivieren Sie TTS
#

tts.enable=false


############################################################
## Release-Modus (App-Installationsmodus)

#
# Release-Modus aktivieren (schreibt Speicherdaten in AppData)
#

release_mode.enable=false
