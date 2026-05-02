animado
=====

Anime es una función para reproducir animaciones basadas en capas a través de la etiqueta anime.

## Archivo de anime

Un archivo anime es un archivo de texto que describe secuencias de transformaciones de capas.

## Secuencia

Para mover el cuadro de mensaje 100 píxeles a la derecha en un segundo, escriba la siguiente secuencia en un archivo de anime.

```
# A block describes a sequence for animation.
# The name of a block can be whatever you like and it won't affect anything.
move {
    # This is a layer specifier.
    layer: msg;

    # These are time specifiers. (in second)
    start: 0.0;
    end: 1.0;

    # These are origin position specifiers. 'r0' means relative '0'.
    from-x: r0;
    from-y: r0;

    # This is an origin alpha specifier.
    from-a: 255;

    # These are final position specifiers. 'r100' means relative '100'.
    to-x: r100;
    to-y: r0;

    # This is a final alpha specifier.
    to-a: 255;
}
```

## Estructura de capas

La siguiente es nuestra estructura de capas en orden de abajo hacia arriba.

| Nombre de capa | Description |
|------------------|--------------------------------------------|
| bg | background |
| bg2 | segundo fondo (para un desplazamiento perfecto) |
| efb1 | efecto en la espalda 1 |
| efb2 | efecto en la espalda 2 |
| efb3 | efecto en la espalda 3 |
| efb4 | efecto en la espalda 4 |
| chb | personaje en el centro trasero |
| chb-eye | personaje en el centro trasero |
| chb-lip | personaje en el centro trasero |
| chl | personaje de la izquierda |
| chl-eye | personaje de la izquierda |
| chl-lip | personaje de la izquierda |
| chlc | personaje en el centro izquierda |
| chlc-eye | personaje en el centro izquierda |
| chlc-lip | personaje en el centro izquierda |
| chr | personaje de la derecha |
| chr-eye | personaje de la derecha |
| chr-lip | personaje de la derecha |
| chrc | personaje en el centro derecho |
| chrc-eye | personaje en el centro derecho |
| chrc-lip | personaje en el centro derecho |
| eff1 | efecto en el frente 1 |
| eff2 | efecto en el frente 2 |
| eff3 | efecto en el frente 3 |
| eff4 | efecto en el frente 4 |
| msgbox | cuadro de mensaje |
| namebox | cuadro de nombre |
| choose1-idle | elija la casilla 1 (inactiva) |
| choose1-hover | elija la casilla 1 (pase el cursor sobre) |
| choose2-idle | elija la casilla 2 (inactiva) |
| choose2-hover | elija la casilla 2 (pase el cursor sobre) |
| choose3-idle | elija la casilla 3 (inactiva) |
| choose3-hover | elija la casilla 3 (pase el cursor sobre) |
| choose4-idle | elija la casilla 4 (inactiva) |
| choose4-hover | elija la casilla 4 (pase el cursor sobre) |
| choose5-idle | elija la casilla 5 (inactiva) |
| choose5-hover | elija la casilla 5 (pase el cursor sobre) |
| choose6-idle | elija la casilla 6 (inactiva) |
| choose6-hover | elija la casilla 6 (pase el cursor sobre) |
| choose7-idle | elija la casilla 7 (inactiva) |
| choose7-hover | elija la casilla 7 (pase el cursor sobre) |
| choose8-idle | elija la casilla 8 (inactiva) |
| choose8-hover | elija la casilla 8 (pase el cursor sobre) |
| chf | cara del personaje |
| chf-eye | cara del personaje |
| chf-lip | cara del personaje |
| click | haga clic en animación |
| auto | banner de modo automático |
| skip | banner del modo de salto |
| text1 | capa de texto 1 |
| text2 | capa de texto 2 |
| text3 | capa de texto 3 |
| text4 | capa de texto 4 |
| text5 | capa de texto 5 |
| text6 | capa de texto 6 |
| text7 | capa de texto 7 |
| text8 | capa de texto 8 |
| gui-button-1 | ID del botón GUI 1 |
| gui-button-2 | ID del botón GUI 2 |
| gui-button-3 | ID del botón GUI 3 |
| gui-button-4 | ID del botón GUI 4 |
| gui-button-5 | ID del botón GUI 5 |
| gui-button-6 | ID del botón GUI 6 |
| gui-button-7 | ID del botón GUI 7 |
| gui-button-8 | ID del botón GUI 8 |
| gui-button-9 | ID del botón GUI 9 |
| gui-button-10 | ID del botón GUI 10 |
| gui-button-11 | ID del botón GUI 11 |
| gui-button-12 | ID del botón GUI 12 |
| gui-button-13 | ID del botón GUI 13 |
| gui-button-14 | ID del botón GUI 14 |
| gui-button-15 | ID del botón GUI 15 |
| gui-button-16 | ID del botón GUI 16 |
| gui-button-17 | ID del botón GUI 17 |
| gui-button-18 | ID del botón GUI 18 |
| gui-button-19 | ID del botón GUI 19 |
| gui-button-20 | ID del botón GUI 20 |
| gui-button-21 | ID del botón GUI 21 |
| gui-button-22 | ID del botón GUI 22 |
| gui-button-23 | ID del botón GUI 23 |
| gui-button-24 | ID del botón GUI 24 |
| gui-button-25 | ID del botón GUI 25 |
| gui-button-26 | ID del botón GUI 26 |
| gui-button-27 | ID del botón GUI 27 |
| gui-button-28 | ID del botón GUI 28 |
| gui-button-29 | ID del botón GUI 29 |
| gui-button-30 | ID del botón GUI 30 |
| gui-button-31 | ID del botón GUI 31 |
| gui-button-32 | ID del botón GUI 32 |

## Escalado y rotación

```
# Scale-up and rotate the `effect1` layer to 2.0x and 360 deg in 3 seconds.
test1 {
    layer: effect1;
 
    start: 0.0;
    end: 3.0;

    from-x: 0;
    from-y: 400;
    from-a: 255;

    to-x: 0;
    to-y: 400;
    to-a: 255;

    # Scaling and rotation origin
    center-x: 600;
    center-y: 100;

    # Scaling factors
    from-scale-x: 1.0;
    from-scale-y: 1.0;
    to-scale-x: 2.0;
    to-scale-y: 2.0;
 
    # Rotation (+ for right, - for left)
    from-rotate: 0.0;
    to-rotate: -360;
}

# Reverse.
test2 {
    layer: effect1;

    start: 3.0;
    end: 6.0;

    from-x: 0;
    from-y: 400;
    from-a: 255;

    to-x: 0;
    to-y: 400;
    to-a: 255;

    center-x: 600;
    center-y: 100;

    from-scale-x: 2.0;
    to-scale-x: 1.0;

    from-scale-y: 2.0;
    to-scale-y: 1.0;

    from-rotate: -360;
    to-rotate: 0;
}
```
