WIP

## Game Settings

| Name                       | Type       | Description                                                   |
|----------------------------|------------|---------------------------------------------------------------|
| game.title.en              | String     | Game title in English.                                        |
| game.novel                 | Boolean    | Enable novel mode.                                            |
| game.locale                | String     | Force the language over the system locale setting.            |

### game.title.en

### game.novel

### game.local

Language setting.

- "":   Use the system setting
- "ja": Fix to Japanese
- "en": Fix to English

--

## Font File Settings

# Font 1
#  - Default font
#  - You can choose this font by adding \f{1} to a message
font.ttf1=system/font/rounded-l-mplus-1c-bold.ttf

# Font 2
#  - You can choose this font by adding \f{2} to a message
#font.ttf2=

# Font 3
#  - You can choose this font by adding \f{3} to a message
#font.ttf3=

# Font 4
#  - You can choose this font by adding \f{4} to a message
#font.ttf4=


############################################################
## Message Box Settings

#
# Image of the message box
#

msgbox.image=system/message/msgbox.png

#
# Animation
#

msgbox.anime.hide=system/message/msgbox-hide.anime
msgbox.anime.show=system/message/msgbox-show.anime

#
# Position of the message box
#

msgbox.x=0
msgbox.y=520

#
# Margins for message box text in pixels
#

msgbox.margin.left=200
msgbox.margin.top=20
msgbox.margin.right=200
msgbox.margin.bottom=30

#
# Line margin for message box text in pixels
# (including character height)
#

msgbox.margin.line=40

#
# Character margin for message box text in pixels
#

msgbox.margin.char=0

#
# Font selection for the message box text (1,2,3,4)
#

msgbox.font.select=1

#
# Font size of the message box text
#

msgbox.font.size=36

#
# Font color of the message box text
#

msgbox.font.r=255
msgbox.font.g=255
msgbox.font.b=255

#
# Font outline width and color of the text of the message box
#

msgbox.font.outline.width=0
msgbox.font.outline.r=0
msgbox.font.outline.g=0
msgbox.font.outline.b=0

#
# Ruby size of the text of the message box
#

msgbox.font.ruby=10

#
# Enable vertical writing of the text of the message box
#

msgbox.font.tategaki=false

#
# Enable character backgroun fill
#

msgbox.fill.enable=false
msgbox.fill.r=255
msgbox.fill.g=255
msgbox.fill.b=255

#
# Enable dimming for the previous paragpaphs of the text of the message box
#

msgbox.dim.enable=false

#
# Dimming color of the text of the message box
#

msgbox.dim.r=0
msgbox.dim.g=0
msgbox.dim.b=0

#
# Dimming outline width and color of the text of the message box
#
msgbox.dim.outline.width=0
msgbox.dim.outline.r=0
msgbox.dim.outline.g=0
msgbox.dim.outline.b=0

#
# Enable font color change of the seen messages of the message box
#

msgbox.seen.enable=false

#
# Font color of the seen messages of the message box
#

msgbox.seen.r=0
msgbox.seen.g=0
msgbox.seen.b=0

#
# Font outline width and color of the seen messages of the message box
#

msgbox.seen.outline.width=0
msgbox.seen.outline.r=0
msgbox.seen.outline.g=0
msgbox.seen.outline.b=0

#
# Enable skip for unseen messages
#

msgbox.skip_unseen=false

############################################################
## Name Box Settings

#
# Enable the name box
#  - Disable the name box if you need the full screen novel style
#

namebox.enable=true

#
# Image of the name box
#

namebox.image=system/message/namebox.png

#
# Animation
#

namebox.anime.hide=system/message/namebox-hide.anime
namebox.anime.show=system/message/namebox-show.anime

#
# Position of the name box
#

namebox.x=80
namebox.y=450

#
# Margins of the text of the name box in pixel
#

namebox.margin.top=5
namebox.margin.left=0

#
# Enable centering of the text of the name box
#

namebox.centering=true

#
# Font selection of the text of the name box (1,2,3,4)
#

namebox.font.select=1

#
# Font size of the text of the name box
#

namebox.font.size=36

#
# Font color of the text of the name box
#

namebox.font.r=255
namebox.font.g=255
namebox.font.b=255

#
# Font outline width and color of the text of the name box
#

namebox.font.outline.width=0
namebox.font.outline.r=255
namebox.font.outline.g=255
namebox.font.outline.b=255

#
# Ruby size of the text of the name box
#

namebox.font.ruby=10

#
# Enable vertical writing of the text of the name box
#

namebox.font.tategaki=false


############################################################
## Click Animation Settings

#
# Position of the click animation
#

click.x=1060
click.y=660

#
# Interval of the click animation
#

click.interval=1.0

#
# Images of the click animation
#

click.image1=system/message/click1.png
click.image2=system/message/click2.png
#click.image3=
#click.image4=
#click.image5=
#click.image6=
#click.image7=
#click.image8=
#click.image9=
#click.image10=
#click.image11=
#click.image12=
#click.image13=
#click.image14=
#click.image15=
#click.image16=

#
# Enable auto move of the click animation
#

click.move=false


############################################################
## Choose Box Settings

#
# Font selection of the text of the choose boxes
#

choose.font.select=1

#
# Font size of the the text of the choose boxes
#

choose.font.size=36

#
# Font color of the text of the non-pointed choose box
#

choose.font.idle.r=255
choose.font.idle.g=255
choose.font.idle.b=255

#
# Font outline width and color of the text of the non-pointed choose box
#

choose.font.idle.outline.width=0
choose.font.idle.outline.r=255
choose.font.idle.outline.g=255
choose.font.idle.outline.b=255

#
# Font color of the text of the pointed choose box
#

choose.font.hover.r=255
choose.font.hover.g=0
choose.font.hover.b=0

#
# Font outline width and color of the text of the pointed choose box
#

choose.font.hover.outline.width=0
choose.font.hover.outline.r=255
choose.font.hover.outline.g=255
choose.font.hover.outline.b=255

#
# Ruby size of the text of the choose boxes
#

choose.font.ruby=10

#
# Enable vertical writing of the text of the choose boxes
#

choose.font.tategaki=false

#
# Sound effect for when the pointed choose box is changed
#

choose.change_se=system/choose/button.ogg

#
# Sound effect for when a choose box is selected
#

choose.click_se=system/choose/button.ogg

#
# Settings for the choose box 1
#

choose.box1.idle=system/choose/idle.png
choose.box1.hover=system/choose/hover.png
choose.box1.x=200
choose.box1.y=130
choose.box1.margin.top=15
choose.box1.idle_anime=system/choose/idle.anime
choose.box1.hover_anime=system/choose/hover.anime

#
# Settings for the choose box 2
#

choose.box2.idle=system/choose/idle.png
choose.box2.hover=system/choose/hover.png
choose.box2.x=200
choose.box2.y=220
choose.box2.margin.top=15
choose.box2.idle_anime=system/choose/idle.anime
choose.box2.hover_anime=system/choose/hover.anime

#
# Settings for the choose box 3
#

choose.box3.idle=system/choose/idle.png
choose.box3.hover=system/choose/hover.png
choose.box3.x=200
choose.box3.y=310
choose.box3.margin.top=15
choose.box3.idle_anime=system/choose/idle.anime
choose.box3.hover_anime=system/choose/hover.anime

#
# Settings for the choose box 4
#

choose.box4.idle=system/choose/idle.png
choose.box4.hover=system/choose/hover.png
choose.box4.x=200
choose.box4.y=400
choose.box4.margin.top=15
choose.box4.idle_anime=system/choose/idle.anime
choose.box4.hover_anime=system/choose/hover.anime

#
# Settings for the choose box 5
#

choose.box5.idle=system/choose/idle.png
choose.box5.hover=system/choose/hover.png
choose.box5.x=200
choose.box5.y=490
choose.box5.margin.top=15
choose.box5.idle_anime=system/choose/idle.anime
choose.box5.hover_anime=system/choose/hover.anime

#
# Settings for the choose box 6
#

choose.box6.idle=system/choose/idle.png
choose.box6.hover=system/choose/hover.png
choose.box6.x=200
choose.box6.y=580
choose.box6.margin.top=15
choose.box6.idle_anime=system/choose/idle.anime
choose.box6.hover_anime=system/choose/hover.anime

#
# Settings for the choose box 7
#

choose.box7.idle=system/choose/idle.png
choose.box7.hover=system/choose/hover.png
choose.box7.x=200
choose.box7.y=670
choose.box7.margin.top=15
choose.box7.idle_anime=system/choose/idle.anime
choose.box7.hover_anime=system/choose/hover.anime

#
# Settings for the choose box 8
#

choose.box8.idle=system/choose/idle.png
choose.box8.hover=system/choose/hover.png
choose.box8.x=200
choose.box8.y=760
choose.box8.margin.top=15
choose.box8.idle_anime=system/choose/idle.anime
choose.box8.hover_anime=system/choose/hover.anime


############################################################
## Save Data Settings
############################################################

#
# Size of the save thumbnail
#

save.thumb.width=213
save.thumb.height=120

#
# "New" mark image
#

#save.new_image=system/save/new.png


############################################################
## System Button Settings

#
# Enable the system button
#

sysbtn.enable=true

#
# Images of the system button
#

sysbtn.idle=system/sysbtn/sysbtn-idle.png
sysbtn.hover=system/sysbtn/sysbtn-hover.png

#
# Anime
#

# Hidden.
sysbtn.anime.out=system/sysbtn/anime-out.anime

# Appearing.
sysbtn.anime.fadein=system/sysbtn/anime-fadein.anime

# Appeared but not pointed.
sysbtn.anime.appear=system/sysbtn/anime-appear.anime

# Hovered.
sysbtn.anime.hover=system/sysbtn/anime-hover.anime

# Disappearing.
sysbtn.anime.fadeout=system/sysbtn/anime-fadeout.anime

#
# Position of the system button
#

sysbtn.x=1183
sysbtn.y=48
sysbtn.width=100
sysbtn.height=100

#
# Sound effects of the system button
#

#sysbtn.enter_se=se/click.ogg
#sysbtn.leave_se=se/click.ogg
#sysbtn.click_se=se/click.ogg


############################################################
## Auto Mode Settings

#
# Image of the auto mode banner
#

automode.image=system/message/auto.png

#
# Animation
#

automode.anime.hide=system/message/auto-hide.anime
automode.anime.show=system/message/auto-show.anime

#
# Position of the auto mode banner
#

automode.x=0
automode.y=126

#
# Sound effect for when entering the auto mode
#

#automode.enter_se=

#
# Sound effect for when leaving the auto mode
#

#automode.leave_se=


############################################################
## Skip Mode Settings

#
# Image of the skip mode banner
#

skipmode.image=system/message/skip.png

#
# Animation
#

skipmode.anime.hide=system/message/skip-hide.anime
skipmode.anime.show=system/message/skip-show.anime

#
# Position of the auto mode banner
#

skipmode.x=0
skipmode.y=126

#
# Sound effect for when entering the skip mode
#

#skipmode.enter_se=

#
# Sound effect for when leaving the auto mode
#

#skipmode.leave_se=


############################################################
## GUI Settings

#
# Font selection for the slot index text of the save/load GUI items
#
gui.save.index.font.select=1
gui.save.index.font.size=30
gui.save.index.font.r=255
gui.save.index.font.g=200
gui.save.index.font.b=200
gui.save.index.font.outline.width=0
gui.save.index.font.outline.r=0
gui.save.index.font.outline.g=0
gui.save.index.font.outline.b=0
gui.save.index.font.ruby=10
gui.save.index.font.tategaki=false
gui.save.index.margin.char=3

#
# Font selection for the date text of the save/load GUI items
#
gui.save.date.font.select=1
gui.save.date.font.size=30
gui.save.date.font.r=255
gui.save.date.font.g=255
gui.save.date.font.b=255
gui.save.date.font.outline.width=0
gui.save.date.font.outline.r=0
gui.save.date.font.outline.g=0
gui.save.date.font.outline.b=0
gui.save.date.font.ruby=10
gui.save.date.font.tategaki=false
gui.save.date.margin.char=5

#
# Font selection for the chapter text of the save/load GUI items
#
gui.save.chapter.font.select=1
gui.save.chapter.font.size=32
gui.save.chapter.font.r=255
gui.save.chapter.font.g=255
gui.save.chapter.font.b=255
gui.save.chapter.font.outline.width=2
gui.save.chapter.font.outline.r=32
gui.save.chapter.font.outline.g=32
gui.save.chapter.font.outline.b=32
gui.save.chapter.font.ruby=10
gui.save.chapter.font.tategaki=false
gui.save.chapter.margin.char=5

#
# Font selection for the message text of the save/load GUI items
#
gui.save.msg.font.select=1
gui.save.msg.font.size=22
gui.save.msg.font.r=255
gui.save.msg.font.g=255
gui.save.msg.font.b=255
gui.save.msg.font.outline.width=1
gui.save.msg.font.outline.r=32
gui.save.msg.font.outline.g=32
gui.save.msg.font.outline.b=32
gui.save.msg.font.ruby=10
gui.save.msg.font.tategaki=false
gui.save.msg.margin.line=30
gui.save.msg.margin.char=0
gui.save.msg.multiline=true

#
# Font selection of the name and the text of the history GUI items
#

gui.history.name.font.select=1
gui.history.name.font.size=34
gui.history.name.font.r=255
gui.history.name.font.g=245
gui.history.name.font.b=245
gui.history.name.font.outline.width=1
gui.history.name.font.outline.r=32
gui.history.name.font.outline.g=32
gui.history.name.font.outline.b=32
gui.history.name.font.ruby=10
gui.history.name.font.tategaki=false
gui.history.name.margin.line=40
gui.history.name.margin.char=0

gui.history.text.font.select=1
gui.history.text.font.size=32
gui.history.text.font.r=255
gui.history.text.font.g=255
gui.history.text.font.b=255
gui.history.text.font.outline.width=1
gui.history.text.font.outline.r=32
gui.history.text.font.outline.g=32
gui.history.text.font.outline.b=32
gui.history.text.font.ruby=10
gui.history.text.font.tategaki=false
gui.history.text.margin.line=40
gui.history.text.margin.char=0

#
# Line quotation of the text of the history GUI items
#

gui.history.quote.name_separator=\n
gui.history.quote.start="
gui.history.quote.end="

#
# Hide the last history item
#

gui.history.hide_last=false

#
# Font selection of the text of the text preview GUI item (1,2,3,4)
#

gui.preview.font.select=1

#
# Font size of the text of the text preview GUI item
#

gui.preview.font.size=36

#
# Font color of the text of the text preview GUI item
#

gui.preview.font.r=255
gui.preview.font.g=255
gui.preview.font.b=255

#
# Font outline width and color of the text of the text preview GUI item
#

gui.preview.font.outline.width=0
gui.preview.font.outline.r=255
gui.preview.font.outline.g=255
gui.preview.font.outline.b=255

#
# Ruby size of the text of the text preview GUI item
#

gui.preview.font.ruby=10

#
# Enable vertical writing of the text of the text preview GUI item
#

gui.preview.font.tategaki=false


############################################################
## Sound Settings

#
# Initial value of the BGM track volume
#

sound.vol.bgm=1.0

#
# Initial value of the voice track volume
#

sound.vol.voice=1.0

#
# Initial value of the SE track volume
#

sound.vol.se=1.0

#
# Initial value of the per-character volumes
#

sound.vol.per_character=1.0


############################################################
## Character Settings

#
# Character names
#  - for name translation, lip sync, auto focus, etc.
#

character.name1=Midori
character.name1.en=Midori
character.name1.zh-cn=美登利
character.name1.zh-tw=美登利
character.name1.ja=みどり

character.name2=Xiaoling
character.name2.en=Xiaoling
character.name2.zh-cn=小玲
character.name2.zh-tw=小玲
character.name2.ja=シャオリン

#character.name3=
#character.name4=
#character.name5=
#character.name6=
#character.name7=
#character.name8=
#character.name9=
#character.name10=
#character.name11=
#character.name12=
#character.name13=
#character.name14=
#character.name15=
#character.name16=
#character.name17=
#character.name18=
#character.name19=
#character.name20=
#character.name21=
#character.name22=
#character.name23=
#character.name24=
#character.name25=
#character.name26=
#character.name27=
#character.name28=
#character.name29=
#character.name30=
#character.name31=
#character.name32=

#
# Character image sub-folders (for lip sync and automatic focus)
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
# Eye blinking frame length in seconds
#

character.eyeblink.frame=0.05

#
# Lip synchronization frame length in seconds
#

character.lipsync.frame=0.04

#
# Lip synchronization frame per characters
#

character.lipsync.chars=14

############################################################
## Auto focus settings
##
## Note: Not working yet on RC1

#
# Auto focus on speaker / unfocus non-speakers on [text] with name
#
#autofocus.on_text_name=true

#
# Auto unfocus all characters on [text] without name
#
#autofocus.on_text_no_name=true

#
# Auto unfocus non-speakers on [ch]
#
#autofocus.on_ch=true

#
# Auto unfocus all characters on [choose]
#
#autofocus.on_choose=true


############################################################
## Stage Settings

#
# Character margins in pixels
#

stage.ch_margin.bottom=0
stage.ch_margin.left=0
stage.ch_margin.right=0


############################################################
## Kira Kira Effect Settings

#
# Enable Kira Kira Effect (click effect animation)
#

kirakira.enable=false

#
# Enable add-blend for Kira Kira Effect
#

kirakira.add_blend=false

#
# Frame length for Kira Kira Effect in seconds
#

kirakira.frame=0.333

#
# Images for Kira Kira Effect
#

#kirakira.image1=kira1.png
#kirakira.image2=kira2.png
#kirakira.image3=kira3.png
#kirakira.image4=kira4.png
#kirakira.image5=
#kirakira.image6=
#kirakira.image7=
#kirakira.image8=
#kirakira.image9=
#kirakira.image10=
#kirakira.image11=
#kirakira.image12=
#kirakira.image13=
#kirakira.image14=
#kirakira.image15=
#kirakira.image16=


############################################################
## Emoji

#
# Emojis (1 to 32)
#

emoji.name1=heart
emoji.image1=system/emoji/heart.png

emoji.name2=sweat
emoji.image2=system/emoji/sweat.png


############################################################
## Text-To-Speech

#
# Enable TTS
#

tts.enable=false


############################################################
## Release Mode (Install App Mode)

#
# Enable Release Mode (writes save data to AppData)
#

release_mode.enable=false
