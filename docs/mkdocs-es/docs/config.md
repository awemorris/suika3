WIP

## Configuración del juego

| Nombre | Tipo | Descripciﾃｳn |
|----------------------------|------------|---------------------------------------------------------------|
| game.title.en | String | Título del juego en inglés.                                        |
| game.novel | Boolean | Habilite el modo novela.                                            |
| game.locale | String | Fuerce el idioma sobre la configuración regional del sistema.            |

### game.title.en

### game.novela

### game.locale

Configuración de idioma.

- ": Utilice la configuración del sistema
- ja: arreglado en japonés
- es: arreglar al inglés

--

## Configuración del archivo de fuente

# Fuente 1
# - Fuente predeterminada
# - Puedes elegir esta fuente agregando \f{1} a un mensaje
font.ttf1=system/font/redondeado-l-mplus-1c-bold.ttf

# Fuente 2
# - Puedes elegir esta fuente agregando \f{2} a un mensaje
#font.ttf2=

# Fuente 3
# - Puedes elegir esta fuente agregando \f{3} a un mensaje
#font.ttf3=

# Fuente 4
# - Puedes elegir esta fuente agregando \f{4} a un mensaje
#font.ttf4=


############################################################
## Configuración del cuadro de mensajes

#
# Imagen del cuadro de mensaje
#

msgbox.image=sistema/mensaje/msgbox.png

#
# Animación
#

msgbox.anime.hide=sistema/message/msgbox-hide.anime
msgbox.anime.show=sistema/message/msgbox-show.anime

#
# Posición del cuadro de mensaje
#

msgbox.x=0
msgbox.y=520

#
# Márgenes para el texto del cuadro de mensaje en píxeles
#

msgbox.margin.left=200
msgbox.margin.top=20
msgbox.margin.right=200
msgbox.margin.bottom=30

#
# Margen de línea para el texto del cuadro de mensaje en píxeles
# (incluida la altura del carácter)
#

msgbox.margin.line=40

#
# Margen de caracteres para el texto del cuadro de mensaje en píxeles
#

msgbox.margin.char=0

#
# Selección de fuente para el texto del cuadro de mensaje (1,2,3,4)
#

msgbox.font.select=1

#
# Tamaño de fuente del texto del cuadro de mensaje
#

msgbox.font.size=36

#
# Color de fuente del texto del cuadro de mensaje
#

msgbox.font.r=255
msgbox.font.g=255
msgbox.font.b=255

#
# Ancho del contorno de fuente y color del texto del cuadro de mensaje
#

msgbox.font.outline.width=0
msgbox.font.outline.r=0
msgbox.font.outline.g=0
msgbox.font.outline.b=0

#
# Tamaño Ruby del texto del cuadro de mensaje.
#

msgbox.font.ruby=10

#
# Habilitar la escritura vertical del texto del cuadro de mensaje.
#

msgbox.font.tategaki=falso

#
# Habilitar relleno de fondo de caracteres
#

msgbox.fill.enable=falso
msgbox.fill.r=255
msgbox.fill.g=255
msgbox.fill.b=255

#
# Habilitar la atenuación de los párrafos anteriores del texto del cuadro de mensaje
#

msgbox.dim.enable=falso

#
# Atenuación del color del texto del cuadro de mensaje.
#

msgbox.dim.r=0
msgbox.dim.g=0
msgbox.dim.b=0

#
# Atenuar el ancho del contorno y el color del texto del cuadro de mensaje
#
msgbox.dim.outline.width=0
msgbox.dim.outline.r=0
msgbox.dim.outline.g=0
msgbox.dim.outline.b=0

#
# Habilitar el cambio de color de fuente de los mensajes vistos en el cuadro de mensaje
#

msgbox.seen.enable=falso

#
# Color de fuente de los mensajes vistos en el cuadro de mensajes.
#

msgbox.visto.r=0
msgbox.visto.g=0
msgbox.visto.b=0

#
# Ancho del contorno de fuente y color de los mensajes vistos en el cuadro de mensaje
#

msgbox.seen.outline.width=0
msgbox.seen.outline.r=0
msgbox.seen.outline.g=0
msgbox.visto.esquema.b=0

#
# Habilitar omitir mensajes no vistos
#

msgbox.skip_unseen=falso

############################################################
## Configuración del cuadro de nombre

#
# Habilitar el cuadro de nombre
# - Desactive el cuadro de nombre si necesita el estilo de novela en pantalla completa
#

namebox.enable=verdadero

#
# Imagen del cuadro de nombre
#

namebox.image=sistema/mensaje/namebox.png

#
# Animación
#

namebox.anime.hide=sistema/mensaje/namebox-hide.anime
namebox.anime.show=sistema/mensaje/namebox-show.anime

#
# Posición del cuadro de nombre
#

cuadro de nombres.x=80
cuadro de nombres.y=450

#
# Márgenes del texto del cuadro de nombre en píxeles
#

cuadro de nombre.margin.top=5
cuadro de nombre.margen.izquierda=0

#
# Habilitar el centrado del texto del cuadro de nombre.
#

namebox.centering=verdadero

#
# Selección de fuente del texto del cuadro de nombre (1,2,3,4)
#

cuadro de nombre.font.select=1

#
# Tamaño de fuente del texto del cuadro de nombre
#

cuadro de nombre.font.size=36

#
# Color de fuente del texto del cuadro de nombre
#

cuadro de nombre.font.r=255
cuadro de nombre.font.g=255
cuadro de nombre.font.b=255

#
# Ancho del contorno de fuente y color del texto del cuadro de nombre
#

cuadro de nombre.font.outline.width=0
cuadro de nombre.font.outline.r=255
cuadro de nombre.font.outline.g=255
cuadro de nombre.font.outline.b=255

#
# Tamaño Ruby del texto del cuadro de nombre.
#

cuadro de nombre.font.ruby=10

#
# Habilitar la escritura vertical del texto del cuadro de nombre.
#

cuadro de nombre.font.tategaki=falso


############################################################
## Haga clic en Configuración de animación

#
# Posición de la animación del clic
#

clic.x=1060
clic.y=660

#
# Intervalo de la animación del clic
#

clic.intervalo=1.0

#
# Imágenes de la animación del clic
#

click.image1=sistema/mensaje/click1.png
click.image2=sistema/mensaje/click2.png
#hacer clic.imagen3=
#hacer clic.imagen4=
#hacer clic.imagen5=
#hacer clic.imagen6=
#hacer clic.imagen7=
#hacer clic.imagen8=
#hacer clic.imagen9=
#hacer clic.imagen10=
#hacer clic.imagen11=
#hacer clic.imagen12=
#hacer clic.imagen13=
#hacer clic.imagen14=
#hacer clic.imagen15=
#hacer clic.imagen16=

#
# Habilitar el movimiento automático de la animación del clic.
#

hacer clic.move=falso


############################################################
## Elija la configuración del cuadro

#
# Selección de fuente del texto de los cuadros de elección.
#

elegir.font.select=1

#
# Tamaño de fuente del texto de los cuadros de elección
#

elegir.font.size=36

#
# Color de fuente del texto del cuadro de selección no puntiagudo
#

elegir.font.idle.r=255
elegir.font.idle.g=255
elegir.font.idle.b=255

#
# Ancho del contorno de fuente y color del texto del cuadro de selección no puntiagudo
#

elegir.font.idle.outline.width=0
elegir.font.idle.outline.r=255
elegir.font.idle.outline.g=255
elegir.font.idle.outline.b=255

#
# Color de fuente del texto del cuadro de selección puntiagudo
#

elegir.font.hover.r=255
elegir.font.hover.g=0
elegir.font.hover.b=0

#
# Ancho del contorno de fuente y color del texto del cuadro de selección puntiagudo
#

elegir.font.hover.outline.width=0
elegir.font.hover.outline.r=255
elegir.font.hover.outline.g=255
elegir.font.hover.outline.b=255

#
# Tamaño Ruby del texto de los cuadros de elección.
#

elegir.font.ruby=10

#
# Habilitar la escritura vertical del texto de los cuadros de elección.
#

elegir.font.tategaki=false

#
# Efecto de sonido para cuando se cambia el cuadro de selección puntiagudo
#

elegir.change_se=system/choose/button.ogg

#
# Efecto de sonido para cuando se selecciona un cuadro de elección
#

elegir.click_se=system/choose/button.ogg

#
# Configuraciones para el cuadro de elección 1
#

elegir.box1.idle=sistema/elegir/idle.png
elegir.box1.hover=sistema/elegir/hover.png
elegir.box1.x=200
elegir.box1.y=130
elegir.box1.margin.top=15
elegir.box1.idle_anime=system/choose/idle.anime
elegir.box1.hover_anime=system/choose/hover.anime

#
# Configuraciones para el cuadro de elección 2
#

elegir.box2.idle=sistema/elegir/idle.png
elegir.box2.hover=sistema/elegir/hover.png
elegir.box2.x=200
elegir.box2.y=220
elegir.box2.margin.top=15
elegir.box2.idle_anime=system/choose/idle.anime
elegir.box2.hover_anime=system/choose/hover.anime

#
# Configuraciones para el cuadro de elección 3
#

elegir.box3.idle=sistema/elegir/idle.png
elegir.box3.hover=system/choose/hover.png
elegir.box3.x=200
elegir.box3.y=310
elegir.box3.margin.top=15
elegir.box3.idle_anime=system/choose/idle.anime
elegir.box3.hover_anime=system/choose/hover.anime

#
# Configuraciones para el cuadro de elección 4
#

elegir.box4.idle=sistema/elegir/idle.png
elegir.box4.hover=system/choose/hover.png
elegir.box4.x=200
elegir.box4.y=400
elegir.box4.margin.top=15
elegir.box4.idle_anime=system/choose/idle.anime
elegir.box4.hover_anime=system/choose/hover.anime

#
# Configuraciones para el cuadro de elección 5
#

elegir.box5.idle=sistema/elegir/idle.png
elegir.box5.hover=system/choose/hover.png
elegir.box5.x=200
elegir.box5.y=490
elegir.box5.margin.top=15
elegir.box5.idle_anime=system/choose/idle.anime
elegir.box5.hover_anime=system/choose/hover.anime

#
# Configuraciones para el cuadro de elección 6
#

elegir.box6.idle=sistema/elegir/idle.png
elegir.box6.hover=system/choose/hover.png
elegir.box6.x=200
elegir.box6.y=580
elegir.box6.margin.top=15
elegir.box6.idle_anime=system/choose/idle.anime
elegir.box6.hover_anime=system/choose/hover.anime

#
# Configuraciones para el cuadro de elección 7
#

elegir.box7.idle=sistema/elegir/idle.png
elegir.box7.hover=system/choose/hover.png
elegir.box7.x=200
elegir.box7.y=670
elegir.box7.margin.top=15
elegir.box7.idle_anime=system/choose/idle.anime
elegir.box7.hover_anime=system/choose/hover.anime

#
# Configuraciones para el cuadro de elección 8
#

elegir.box8.idle=sistema/elegir/idle.png
elegir.box8.hover=system/choose/hover.png
elegir.box8.x=200
elegir.box8.y=760
elegir.box8.margin.top=15
elegir.box8.idle_anime=system/choose/idle.anime
elegir.box8.hover_anime=system/choose/hover.anime


############################################################
## Guardar configuración de datos
############################################################

#
# Tamaño de la miniatura guardada
#

guardar.thumb.width=213
guardar.thumb.height=120

#
# Nueva imagen de marca
#

#save.new_image=sistema/save/new.png


############################################################
## Configuración del botón del sistema

#
# Habilitar el botón del sistema
#

sysbtn.enable=verdadero

#
# Imágenes del botón del sistema
#

sysbtn.idle=sistema/sysbtn/sysbtn-idle.png
sysbtn.hover=sistema/sysbtn/sysbtn-hover.png

#
# animado
#

# Oculto.
sysbtn.anime.out=sistema/sysbtn/anime-out.anime

# Apareciendo.
sysbtn.anime.fadein=system/sysbtn/anime-fadein.anime

# Apareció pero no puntiagudo.
sysbtn.anime.appear=system/sysbtn/anime-appear.anime

# Flotado.
sysbtn.anime.hover=system/sysbtn/anime-hover.anime

# Desapareciendo.
sysbtn.anime.fadeout=system/sysbtn/anime-fadeout.anime

#
# Posición del botón del sistema
#

sysbtn.x=1183
sysbtn.y=48
sysbtn.ancho=100
sysbtn.altura=100

#
# Efectos de sonido del botón del sistema.
#

#sysbtn.enter_se=se/click.ogg
#sysbtn.leave_se=se/click.ogg
#sysbtn.click_se=se/click.ogg


############################################################
## Configuración del modo automático

#
# Imagen del banner del modo automático
#

automode.image=sistema/mensaje/auto.png

#
# Animación
#

automode.anime.hide=sistema/mensaje/auto-hide.anime
automode.anime.show=system/message/auto-show.anime

#
# Posición del banner del modo automático
#

modo automático.x=0
modo automático.y=126

#
# Efecto de sonido al ingresar al modo automático.
#

#modoautomático.enter_se=

#
# Efecto de sonido al salir del modo automático.
#

#modoautomático.leave_se=


############################################################
## Configuración del modo de omisión

#
# Imagen del banner del modo de omisión
#

skipmode.image=system/message/skip.png

#
# Animación
#

skipmode.anime.hide=system/message/skip-hide.anime
skipmode.anime.show=system/message/skip-show.anime

#
# Posición del banner del modo automático
#

modo de salto.x=0
modo de salto.y=126

#
# Efecto de sonido para cuando se ingresa al modo de salto
#

#skipmode.enter_se=

#
# Efecto de sonido al salir del modo automático.
#

#skipmode.leave_se=


############################################################
## Configuración de la interfaz gráfica de usuario

#
# Selección de fuente para el texto del índice de ranura de los elementos de la GUI para guardar/cargar
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
# Selección de fuente para el texto de la fecha de los elementos de la GUI para guardar/cargar
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
# Selección de fuente para el texto del capítulo de los elementos de la GUI para guardar/cargar
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
# Selección de fuente para el texto del mensaje de los elementos de la GUI para guardar/cargar
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
gui.save.msg.multiline=verdadero

#
# Selección de fuente del nombre y el texto de los elementos de la GUI del historial.
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
# Cita de línea del texto de los elementos de la GUI del historial
#

gui.history.quote.name_separator=\n
gui.historia.quote.start=
gui.history.quote.end=

#
# Ocultar el último elemento del historial
#

gui.history.hide_last=false

#
# Selección de fuente del texto del elemento GUI de vista previa de texto (1,2,3,4)
#

gui.preview.font.select=1

#
# Tamaño de fuente del texto del elemento GUI de vista previa de texto
#

gui.preview.font.size=36

#
# Color de fuente del texto del elemento GUI de vista previa de texto
#

gui.preview.font.r=255
gui.preview.font.g=255
gui.preview.font.b=255

#
# Ancho del contorno de fuente y color del texto del elemento GUI de vista previa de texto
#

gui.preview.font.outline.width=0
gui.preview.font.outline.r=255
gui.preview.font.outline.g=255
gui.preview.font.outline.b=255

#
# Tamaño Ruby del texto del elemento GUI de vista previa de texto
#

gui.preview.font.ruby=10

#
# Habilitar la escritura vertical del texto del elemento GUI de vista previa de texto
#

gui.preview.font.tategaki=falso


############################################################
## Configuración de sonido

#
# Valor inicial del volumen de la pista BGM
#

sonido.vol.bgm=1.0

#
# Valor inicial del volumen de la pista de voz.
#

sonido.vol.voz=1.0

#
# Valor inicial del volumen de la pista SE
#

sonido.vol.se=1.0

#
# Valor inicial de los volúmenes por carácter.
#

sonido.vol.per_character=1.0


############################################################
## Configuración de caracteres

#
# Nombres de personajes
# - para traducción de nombres, sincronización de labios, enfoque automático, etc.
#

personaje.nombre1=Midori
personaje.name1.en=Midori
personaje.nombre1.zh-cn=美登利
personaje.nombre1.zh-tw=美登利
personaje.nombre1.ja=みどり

personaje.nombre2=Xiaoling
personaje.name2.en=Xiaoling
personaje.nombre2.zh-cn=小玲
personaje.nombre2.zh-tw=小玲
personaje.nombre2.ja=シャオリン

#personaje.nombre3=
#personaje.nombre4=
#personaje.nombre5=
#personaje.nombre6=
#personaje.nombre7=
#personaje.nombre8=
#personaje.nombre9=
#personaje.nombre10=
#personaje.nombre11=
#personaje.nombre12=
#personaje.nombre13=
#personaje.nombre14=
#personaje.nombre15=
#personaje.nombre16=
#personaje.nombre17=
#personaje.nombre18=
#personaje.nombre19=
#personaje.nombre20=
#personaje.nombre21=
#personaje.nombre22=
#personaje.nombre23=
#personaje.nombre24=
#personaje.nombre25=
#personaje.nombre26=
#personaje.nombre27=
#personaje.nombre28=
#personaje.nombre29=
#personaje.nombre30=
#personaje.nombre31=
#personaje.nombre32=

#
# Subcarpetas de imágenes de personajes (para sincronización de labios y enfoque automático)
#

personaje.carpeta1=ch/midori/
personaje.carpeta2=ch/xiaoling/
#carácter.carpeta3=
#carácter.carpeta4=
#carácter.carpeta5=
#carácter.carpeta6=
#carácter.carpeta7=
#carácter.carpeta8=
#carácter.carpeta9=
#carácter.carpeta10=
#carácter.carpeta11=
#carácter.carpeta12=
#carácter.carpeta13=
#carácter.carpeta14=
#carácter.carpeta15=
#carácter.carpeta16=
#carácter.carpeta17=
#carácter.carpeta18=
#carácter.carpeta19=
#carácter.carpeta20=
#carácter.carpeta21=
#carácter.carpeta22=
#carácter.carpeta23=
#carácter.carpeta24=
#carácter.carpeta25=
#carácter.carpeta26=
#carácter.carpeta27=
#carácter.carpeta28=
#carácter.carpeta29=
#carácter.carpeta30=
#carácter.carpeta31=
#carácter.carpeta32=

#
# Intervalo de parpadeo en segundos
#

caracter.intervalo.intervalo=4.0

#
# Longitud del cuadro de parpadeo del ojo en segundos
#

caracter.eyeblink.frame=0.05

#
# Longitud del cuadro de sincronización de labios en segundos
#

personaje.lipsync.frame=0.04

#
# Cuadro de sincronización de labios por personaje
#

carácter.lipsync.chars=14

############################################################
## Configuración de enfoque automático
##
## Nota: Aún no funciona en RC1

#
# Enfoque automático en el hablante / desenfoque a los que no hablan en [texto] con nombre
#
#enfoque automático.on_text_name=true

#
# Desenfoque automático de todos los caracteres en [texto] sin nombre
#
#autofocus.on_text_no_name=true

#
# Desenfoque automático de personas que no hablan en [ch]
#
#enfoque automático.on_ch=true

#
# Desenfoque automático de todos los personajes en [elegir]
#
#enfoque automático.on_choose=true


############################################################
## Configuración del escenario

#
# Márgenes de caracteres en píxeles
#

etapa.ch_margin.bottom=0
etapa.ch_margin.left=0
etapa.ch_margin.right=0


############################################################
## Configuración del efecto Kira Kira

#
# Habilitar el efecto Kira Kira (animación del efecto de clic)
#

kirakira.enable=falso

#
# Habilitar agregar-mezcla para el efecto Kira Kira
#

kirakira.add_blend=false

#
# Longitud del cuadro para el efecto Kira Kira en segundos
#

kirakira.frame=0.333

#
# Imágenes para el efecto Kira Kira
#

#kirakira.image1=kira1.png
#kirakira.image2=kira2.png
#kirakira.image3=kira3.png
#kirakira.image4=kira4.png
#kirakira.image5=
#kirakira.image6=
#kirakira.imagen7=
#kirakira.image8=
#kirakira.image9=
#kirakira.imagen10=
#kirakira.imagen11=
#kirakira.imagen12=
#kirakira.imagen13=
#kirakira.imagen14=
#kirakira.imagen15=
#kirakira.imagen16=


############################################################
##Emojis

#
# Emojis (1 a 32)
#

emoji.nombre1=corazón
emoji.image1=sistema/emoji/corazón.png

emoji.name2=sudor
emoji.image2=sistema/emoji/sudor.png


############################################################
## Texto a voz

#
# Habilitar TTS
#

tts.enable=falso


############################################################
## Modo de lanzamiento (modo de instalación de aplicación)

#
# Habilitar el modo de lanzamiento (escribe los datos guardados en AppData)
#

release_mode.enable=falso
