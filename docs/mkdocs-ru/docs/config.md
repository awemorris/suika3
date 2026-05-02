WIP

## Настройки игры

| Имя                        | Тип        | Описание                                                      |
|----------------------------|------------|---------------------------------------------------------------|
| game.title.en              | String     | Название игры на английском языке.                            |
| game.novel                 | Boolean    | Включить режим новеллы.                                       |
| game.locale                | String     | Принудительно задать язык вместо системной локали.            |

### game.title.en

### game.novel

### game.local

Настройка языка.

- "":   Использовать системную настройку
- "ja": Зафиксировать японский язык
- "en": Зафиксировать английский язык

--

## Настройки файлов шрифтов

# Шрифт 1
#  - Шрифт по умолчанию
#  - Этот шрифт можно выбрать, добавив \f{1} в сообщение
font.ttf1=system/font/rounded-l-mplus-1c-bold.ttf

# Шрифт 2
#  - Этот шрифт можно выбрать, добавив \f{2} в сообщение
#font.ttf2=

# Шрифт 3
#  - Этот шрифт можно выбрать, добавив \f{3} в сообщение
#font.ttf3=

# Шрифт 4
#  - Этот шрифт можно выбрать, добавив \f{4} в сообщение
#font.ttf4=


############################################################
## Настройки окна сообщений

#
# Изображение окна сообщений
#

msgbox.image=system/message/msgbox.png

#
# Анимация
#

msgbox.anime.hide=system/message/msgbox-hide.anime
msgbox.anime.show=system/message/msgbox-show.anime

#
# Позиция окна сообщений
#

msgbox.x=0
msgbox.y=520

#
# Отступы текста окна сообщений в пикселях
#

msgbox.margin.left=200
msgbox.margin.top=20
msgbox.margin.right=200
msgbox.margin.bottom=30

#
# Межстрочный интервал текста окна сообщений в пикселях
# (включая высоту символа)
#

msgbox.margin.line=40

#
# Межсимвольный интервал текста окна сообщений в пикселях
#

msgbox.margin.char=0

#
# Выбор шрифта для текста окна сообщений (1,2,3,4)
#

msgbox.font.select=1

#
# Размер шрифта текста окна сообщений
#

msgbox.font.size=36

#
# Цвет шрифта текста окна сообщений
#

msgbox.font.r=255
msgbox.font.g=255
msgbox.font.b=255

#
# Ширина и цвет обводки шрифта текста окна сообщений
#

msgbox.font.outline.width=0
msgbox.font.outline.r=0
msgbox.font.outline.g=0
msgbox.font.outline.b=0

#
# Размер ruby для текста окна сообщений
#

msgbox.font.ruby=10

#
# Включить вертикальное письмо для текста окна сообщений
#

msgbox.font.tategaki=false

#
# Включить заливку фона символов
#

msgbox.fill.enable=false
msgbox.fill.r=255
msgbox.fill.g=255
msgbox.fill.b=255

#
# Включить затемнение предыдущих абзацев текста окна сообщений
#

msgbox.dim.enable=false

#
# Цвет затемнения текста окна сообщений
#

msgbox.dim.r=0
msgbox.dim.g=0
msgbox.dim.b=0

#
# Ширина и цвет обводки затемнения текста окна сообщений
#
msgbox.dim.outline.width=0
msgbox.dim.outline.r=0
msgbox.dim.outline.g=0
msgbox.dim.outline.b=0

#
# Включить изменение цвета шрифта прочитанных сообщений в окне сообщений
#

msgbox.seen.enable=false

#
# Цвет шрифта прочитанных сообщений в окне сообщений
#

msgbox.seen.r=0
msgbox.seen.g=0
msgbox.seen.b=0

#
# Ширина и цвет обводки шрифта прочитанных сообщений в окне сообщений
#

msgbox.seen.outline.width=0
msgbox.seen.outline.r=0
msgbox.seen.outline.g=0
msgbox.seen.outline.b=0

#
# Включить пропуск непрочитанных сообщений
#

msgbox.skip_unseen=false

############################################################
## Настройки окна имени

#
# Включить окно имени
#  - Отключите окно имени, если нужен полноэкранный стиль новеллы
#

namebox.enable=true

#
# Изображение окна имени
#

namebox.image=system/message/namebox.png

#
# Анимация
#

namebox.anime.hide=system/message/namebox-hide.anime
namebox.anime.show=system/message/namebox-show.anime

#
# Позиция окна имени
#

namebox.x=80
namebox.y=450

#
# Отступы текста окна имени в пикселях
#

namebox.margin.top=5
namebox.margin.left=0

#
# Включить центрирование текста окна имени
#

namebox.centering=true

#
# Выбор шрифта текста окна имени (1,2,3,4)
#

namebox.font.select=1

#
# Размер шрифта текста окна имени
#

namebox.font.size=36

#
# Цвет шрифта текста окна имени
#

namebox.font.r=255
namebox.font.g=255
namebox.font.b=255

#
# Ширина и цвет обводки шрифта текста окна имени
#

namebox.font.outline.width=0
namebox.font.outline.r=255
namebox.font.outline.g=255
namebox.font.outline.b=255

#
# Размер ruby для текста окна имени
#

namebox.font.ruby=10

#
# Включить вертикальное письмо для текста окна имени
#

namebox.font.tategaki=false


############################################################
## Настройки анимации щелчка

#
# Позиция анимации щелчка
#

click.x=1060
click.y=660

#
# Интервал анимации щелчка
#

click.interval=1.0

#
# Изображения анимации щелчка
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
# Включить автоматическое перемещение анимации щелчка
#

click.move=false


############################################################
## Настройки окна выбора

#
# Выбор шрифта текста окон выбора
#

choose.font.select=1

#
# Размер шрифта текста окон выбора
#

choose.font.size=36

#
# Цвет шрифта текста окна выбора без наведения
#

choose.font.idle.r=255
choose.font.idle.g=255
choose.font.idle.b=255

#
# Ширина и цвет обводки шрифта текста окна выбора без наведения
#

choose.font.idle.outline.width=0
choose.font.idle.outline.r=255
choose.font.idle.outline.g=255
choose.font.idle.outline.b=255

#
# Цвет шрифта текста окна выбора при наведении
#

choose.font.hover.r=255
choose.font.hover.g=0
choose.font.hover.b=0

#
# Ширина и цвет обводки шрифта текста окна выбора при наведении
#

choose.font.hover.outline.width=0
choose.font.hover.outline.r=255
choose.font.hover.outline.g=255
choose.font.hover.outline.b=255

#
# Размер ruby для текста окон выбора
#

choose.font.ruby=10

#
# Включить вертикальное письмо для текста окон выбора
#

choose.font.tategaki=false

#
# Звуковой эффект при смене окна выбора под указателем
#

choose.change_se=system/choose/button.ogg

#
# Звуковой эффект при выборе окна выбора
#

choose.click_se=system/choose/button.ogg

#
# Настройки окна выбора 1
#

choose.box1.idle=system/choose/idle.png
choose.box1.hover=system/choose/hover.png
choose.box1.x=200
choose.box1.y=130
choose.box1.margin.top=15
choose.box1.idle_anime=system/choose/idle.anime
choose.box1.hover_anime=system/choose/hover.anime

#
# Настройки окна выбора 2
#

choose.box2.idle=system/choose/idle.png
choose.box2.hover=system/choose/hover.png
choose.box2.x=200
choose.box2.y=220
choose.box2.margin.top=15
choose.box2.idle_anime=system/choose/idle.anime
choose.box2.hover_anime=system/choose/hover.anime

#
# Настройки окна выбора 3
#

choose.box3.idle=system/choose/idle.png
choose.box3.hover=system/choose/hover.png
choose.box3.x=200
choose.box3.y=310
choose.box3.margin.top=15
choose.box3.idle_anime=system/choose/idle.anime
choose.box3.hover_anime=system/choose/hover.anime

#
# Настройки окна выбора 4
#

choose.box4.idle=system/choose/idle.png
choose.box4.hover=system/choose/hover.png
choose.box4.x=200
choose.box4.y=400
choose.box4.margin.top=15
choose.box4.idle_anime=system/choose/idle.anime
choose.box4.hover_anime=system/choose/hover.anime

#
# Настройки окна выбора 5
#

choose.box5.idle=system/choose/idle.png
choose.box5.hover=system/choose/hover.png
choose.box5.x=200
choose.box5.y=490
choose.box5.margin.top=15
choose.box5.idle_anime=system/choose/idle.anime
choose.box5.hover_anime=system/choose/hover.anime

#
# Настройки окна выбора 6
#

choose.box6.idle=system/choose/idle.png
choose.box6.hover=system/choose/hover.png
choose.box6.x=200
choose.box6.y=580
choose.box6.margin.top=15
choose.box6.idle_anime=system/choose/idle.anime
choose.box6.hover_anime=system/choose/hover.anime

#
# Настройки окна выбора 7
#

choose.box7.idle=system/choose/idle.png
choose.box7.hover=system/choose/hover.png
choose.box7.x=200
choose.box7.y=670
choose.box7.margin.top=15
choose.box7.idle_anime=system/choose/idle.anime
choose.box7.hover_anime=system/choose/hover.anime

#
# Настройки окна выбора 8
#

choose.box8.idle=system/choose/idle.png
choose.box8.hover=system/choose/hover.png
choose.box8.x=200
choose.box8.y=760
choose.box8.margin.top=15
choose.box8.idle_anime=system/choose/idle.anime
choose.box8.hover_anime=system/choose/hover.anime


############################################################
## Настройки данных сохранения
############################################################

#
# Размер миниатюры сохранения
#

save.thumb.width=213
save.thumb.height=120

#
# Изображение метки "New"
#

#save.new_image=system/save/new.png


############################################################
## Настройки системной кнопки

#
# Включить системную кнопку
#

sysbtn.enable=true

#
# Изображения системной кнопки
#

sysbtn.idle=system/sysbtn/sysbtn-idle.png
sysbtn.hover=system/sysbtn/sysbtn-hover.png

#
# Анимация
#

# Скрыта.
sysbtn.anime.out=system/sysbtn/anime-out.anime

# Появляется.
sysbtn.anime.fadein=system/sysbtn/anime-fadein.anime

# Появилась, но указатель не наведен.
sysbtn.anime.appear=system/sysbtn/anime-appear.anime

# Указатель наведен.
sysbtn.anime.hover=system/sysbtn/anime-hover.anime

# Исчезает.
sysbtn.anime.fadeout=system/sysbtn/anime-fadeout.anime

#
# Позиция системной кнопки
#

sysbtn.x=1183
sysbtn.y=48
sysbtn.width=100
sysbtn.height=100

#
# Звуковые эффекты системной кнопки
#

#sysbtn.enter_se=se/click.ogg
#sysbtn.leave_se=se/click.ogg
#sysbtn.click_se=se/click.ogg


############################################################
## Настройки авторежима

#
# Изображение баннера авторежима
#

automode.image=system/message/auto.png

#
# Анимация
#

automode.anime.hide=system/message/auto-hide.anime
automode.anime.show=system/message/auto-show.anime

#
# Позиция баннера авторежима
#

automode.x=0
automode.y=126

#
# Звуковой эффект при входе в авторежим
#

#automode.enter_se=

#
# Звуковой эффект при выходе из авторежима
#

#automode.leave_se=


############################################################
## Настройки режима пропуска

#
# Изображение баннера режима пропуска
#

skipmode.image=system/message/skip.png

#
# Анимация
#

skipmode.anime.hide=system/message/skip-hide.anime
skipmode.anime.show=system/message/skip-show.anime

#
# Позиция баннера авторежима
#

skipmode.x=0
skipmode.y=126

#
# Звуковой эффект при входе в режим пропуска
#

#skipmode.enter_se=

#
# Звуковой эффект при выходе из авторежима
#

#skipmode.leave_se=


############################################################
## Настройки GUI

#
# Выбор шрифта для текста индекса слота в элементах GUI сохранения/загрузки
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
# Выбор шрифта для текста даты в элементах GUI сохранения/загрузки
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
# Выбор шрифта для текста главы в элементах GUI сохранения/загрузки
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
# Выбор шрифта для текста сообщения в элементах GUI сохранения/загрузки
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
# Выбор шрифта для имени и текста в элементах GUI истории
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
# Кавычки строк текста в элементах GUI истории
#

gui.history.quote.name_separator=\n
gui.history.quote.start="
gui.history.quote.end="

#
# Скрыть последний элемент истории
#

gui.history.hide_last=false

#
# Выбор шрифта текста элемента GUI предварительного просмотра текста (1,2,3,4)
#

gui.preview.font.select=1

#
# Размер шрифта текста элемента GUI предварительного просмотра текста
#

gui.preview.font.size=36

#
# Цвет шрифта текста элемента GUI предварительного просмотра текста
#

gui.preview.font.r=255
gui.preview.font.g=255
gui.preview.font.b=255

#
# Ширина и цвет обводки шрифта текста элемента GUI предварительного просмотра текста
#

gui.preview.font.outline.width=0
gui.preview.font.outline.r=255
gui.preview.font.outline.g=255
gui.preview.font.outline.b=255

#
# Размер ruby для текста элемента GUI предварительного просмотра текста
#

gui.preview.font.ruby=10

#
# Включить вертикальное письмо для текста элемента GUI предварительного просмотра текста
#

gui.preview.font.tategaki=false


############################################################
## Настройки звука

#
# Начальное значение громкости дорожки BGM
#

sound.vol.bgm=1.0

#
# Начальное значение громкости дорожки голоса
#

sound.vol.voice=1.0

#
# Начальное значение громкости дорожки SE
#

sound.vol.se=1.0

#
# Начальное значение громкости по персонажам
#

sound.vol.per_character=1.0


############################################################
## Настройки персонажей

#
# Имена персонажей
#  - для перевода имен, синхронизации губ, автофокуса и т. п.
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
# Подпапки изображений персонажей (для синхронизации губ и автоматического фокуса)
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
# Интервал моргания в секундах
#

character.eyeblink.interval=4.0

#
# Длительность кадра моргания в секундах
#

character.eyeblink.frame=0.05

#
# Длительность кадра синхронизации губ в секундах
#

character.lipsync.frame=0.04

#
# Кадр синхронизации губ на количество символов
#

character.lipsync.chars=14

############################################################
## Настройки автофокуса
##
## Примечание: пока не работает в RC1

#
# Автофокус на говорящем / снятие фокуса с неговорящих при [text] с именем
#
#autofocus.on_text_name=true

#
# Автоматически снимать фокус со всех персонажей при [text] без имени
#
#autofocus.on_text_no_name=true

#
# Автоматически снимать фокус с неговорящих при [ch]
#
#autofocus.on_ch=true

#
# Автоматически снимать фокус со всех персонажей при [choose]
#
#autofocus.on_choose=true


############################################################
## Настройки сцены

#
# Отступы персонажей в пикселях
#

stage.ch_margin.bottom=0
stage.ch_margin.left=0
stage.ch_margin.right=0


############################################################
## Настройки эффекта Kira Kira

#
# Включить эффект Kira Kira (анимация эффекта щелчка)
#

kirakira.enable=false

#
# Включить аддитивное смешивание для эффекта Kira Kira
#

kirakira.add_blend=false

#
# Длительность кадра эффекта Kira Kira в секундах
#

kirakira.frame=0.333

#
# Изображения для эффекта Kira Kira
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
## Эмодзи

#
# Эмодзи (1-32)
#

emoji.name1=heart
emoji.image1=system/emoji/heart.png

emoji.name2=sweat
emoji.image2=system/emoji/sweat.png


############################################################
## Преобразование текста в речь

#
# Включить синтез речи (TTS)
#

tts.enable=false


############################################################
## Режим выпуска (режим установленного приложения)

#
# Включить режим выпуска (записывает данные сохранения в AppData)
#

release_mode.enable=false
