Menú del sistema
===========

## SysBtn

Suika3 presenta un botón de menú de hamburguesas, generalmente ubicado en el
esquina superior izquierda de la pantalla. Este botón se denomina internamente
como el `System Button (SysBtn)`.

Para garantizar el cumplimiento de las directrices de la App Store, Suika3 evita colocar
pequeños botones estilo PC alrededor de la ventana del mensaje, adoptando un
En su lugar, se debe adoptar un enfoque centrado en los dispositivos móviles. El SysBtn consta de dos imágenes y
animaciones asociadas. Para una experiencia de usuario perfecta, el botón está
Responsivo: aparece al mover el mouse o al tocarlo, y automáticamente
se esconde después de unos segundos de inactividad. Si bien este comportamiento es
codificado para el cumplimiento de la tienda, el SysBtn puede ser completamente
deshabilitado (por ejemplo, para demostraciones o modo quiosco) configurando
`sysbtn.enable=false` en la configuración.

Si bien la ausencia de botones alrededor de la ventana de mensaje familiar puede
se siente inesperado al principio, esperamos que comprenda que esto
La evolución es esencial para adaptar las novelas visuales al móvil moderno.
plataformas.

Por favor vea también:
- `config.ini` (buscar `sysbtn`)
- `system/sysbtn/` (en el juego de muestra)

## MenúSistema

Al hacer clic en SysBtn se activa una GUI conocida como `Menú del sistema
(MenúSistema)`. SysMenu es totalmente personalizable y se puede configurar para
Incluye funciones esenciales como Guardar, Cargar, Modo automático, Modo omitir,
Historial y configuración.

Por favor vea también:
- `system/sysmenu/` (en el juego de muestra)

## Guardar/cargar pantallas

Las pantallas de guardar y cargar son totalmente personalizables mediante archivos GUI.

Por favor vea también:
- `system/save/` (en el juego de muestra)
- `system/load/` (en el juego de muestra)

## Botones de modo automático y modo de salto

Los botones Auto y Saltar utilizan tipos de botones GUI especializados. Estos
Los botones activan sus respectivos modos (modo automático o modo de salto) cuando
hizo clic.

Por favor vea también:
- `system/sysmenu/` (en el juego de muestra)

## Pantalla de historial

La pantalla Historial es totalmente personalizable a través de un archivo GUI.

Por favor vea también:
- `system/history/` (en el juego de muestra)

## Pantalla de configuración

La pantalla de configuración es totalmente personalizable a través de un archivo GUI.

La pantalla de configuración del juego de muestra incluye:
- Controles deslizantes de música de fondo, efectos de sonido y volumen de voz
- Cambio de idioma (EN/JP)
- Ajustes de velocidad en modo texto y automático
- Vista previa de la velocidad del texto

Además, puedes implementar:
- Volumen maestro
- Volumen por carácter
- Cambio de idioma (para todos los idiomas admitidos)
- Botones personalizados

Por favor vea también:
- `system/config/` (en el juego de muestra)
