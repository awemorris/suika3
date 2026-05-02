Ojo parpadeando
============

Una imagen de parpadeo debe almacenarse en la carpeta "ojo" que está
ubicado en la misma carpeta que el image(s) del personaje.

- happy.png (archivo del personaje principal)
- ojo/
    - happy.png (archivo de parpadeo de ojos)

Una imagen de parpadeo consta de frame(s) de parpadeo difference(s). 
Un marco debe tener el mismo tamaño que la imagen del personaje. 
Los fotogramas deben almacenarse horizontalmente en orden de izquierda a derecha. Vea el juego de muestra para ver la imagen real.

Se deben suavizar los valores alfa en los bordes.
Utilice "Desenfocar selección" y "Eliminar selección" en un software de edición de imágenes.

El intervalo de parpadeo de los ojos se puede especificar en el archivo `config.ini`.
Los intervalos son ligeramente aleatorios y, en ocasiones, se producen dobles parpadeos.

```
#
# Eye blinking interval (seconds)
#
character.eyeblink.interval=4.0

#
# Eye blining frame length (seconds per frame)
#
character.eyeblink.frame=0.15
```
