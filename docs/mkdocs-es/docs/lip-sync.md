Sincronización de labios
===================

Una imagen de sincronización de labios debe almacenarse en la carpeta "lip" que está
ubicado en la misma carpeta que el image(s) del personaje.

* happy.png (archivo del personaje principal)
* labio/
    * happy.png (archivo de sincronización de labios)

Una imagen de sincronización de labios consta de frame(s) de sincronización de labios difference(s).
Un marco debe tener el mismo tamaño que la imagen del personaje.
Los fotogramas deben almacenarse horizontalmente en orden de izquierda a derecha.

Se deben suavizar los valores alfa en los bordes.
Utilice "Desenfocar selección" y "Eliminar selección" en un software de edición de imágenes.

El intervalo de sincronización de labios se puede especificar en el archivo `project.txt`.

```
#
# Lip synchronization frame length (seconds per frame)
#
character.lipsync.frame=0.3

#
# Lip synchronization times per characters
#
character.lipsync.chars=3
```
