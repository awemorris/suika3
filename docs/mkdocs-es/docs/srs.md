Especificaciones de requisitos del sistema para Suika3
===========================================

## 1. Descripción general

Suika3 es un tiempo de ejecución de secuencias de comandos de alto rendimiento optimizado para Visual
Novelas (VN) y juegos 2D. Proporciona un entorno DSL de múltiples capas.
para equilibrar la facilidad de uso con la extensibilidad de nivel profesional.

## 2. Componentes principales (la pila DSL)

Suika3 empodera a los creadores a través de cuatro lenguajes especializados:

- NovelML (DSL basado en etiquetas): un lenguaje de marcado sencillo que utiliza
  Etiquetas `[]` para el desarrollo rápido de escenarios VN.

- Anime (Animation DSL): un sistema dedicado para ráster basado en capas
  animaciones de imágenes, controlando secuencias de transformación afines.

- GUI (UI/UX DSL): un conjunto de herramientas flexible para crear pantallas interactivas
   con botones optimizados para los requisitos de VN.

- Ray (General-Purpose Scripting): Un potente scripting
  Idioma con VN API.
    - Personalización: defina etiquetas NovelML personalizadas.
    - Rendimiento: compilado JIT en PC para una iteración rápida; compilado por AOT
      a binarios nativos para el cumplimiento de iOS.
    - Acceso de bajo nivel: enlaces directos a las API de Suika3 Core C.

## 3. Objetivos clave y filosofía de diseño

- Experiencia móvil primero: diseñada con la creencia de que los teléfonos inteligentes
  son un dispositivo informático primario. Evita UI/UX centrada en PC en
  a favor de una sensación móvil nativa.

- Compatibilidad de publicación en tienda: Totalmente compatible con la tienda iOS/Android
  políticas a través de compilación AOT y diseño responsivo.

- Alta portabilidad:
    - Nivel 1: iOS, Android, HarmonyOS NEXT, Windows, macOS, Linux
    - Nivel 2: Consolas de juegos
    - Nivel 3: Chromebook, Wasm (WebAssembly)

- Más allá de las novelas visuales: aunque centradas en VN, la base 2D subyacente
  permite la fusión de géneros (por ejemplo, VN + RPG o Acción).

## 4. Fuera de alcance/restricciones

Para mantener la portabilidad y el rendimiento, Suika3 excluye explícitamente:

- Funciones exclusivas para PC: Suika3 no reemplaza el modelo heredado
  Motores VN solo para PC.

- Implementación web a gran escala: el port Wasm está diseñado para demostraciones, no
  distribución primaria.

- Gráficos 3D: actualmente enfocados en 2D (se planea soporte futuro para 3D)
  junto con la generación de activos impulsada por la IA).

- Middleware propietario: no hay soporte para tecnologías cerradas como
  Live2D para garantizar la máxima portabilidad del motor.

## 5. Novela ML

## 6. Rayo

## 7. Animado

## 8. GUI

## 9. Configuración

