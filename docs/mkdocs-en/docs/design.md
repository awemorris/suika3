Suika3 Design
=============

## Layered Component Model

Suika3 is not a monolithic design, but is separated into small libraries and parts, with an overall hierarchical structure stacked on top of each other.

- Each layer implements only one feature.
- Each layer provides a public C language API to the layer one level above it.
- Each layer can only use the public C language API provided by the layer one level below it.

This type of structure is called the "Layered Component Model" in Suika3.

The advantage of this approach is that while classes in object-oriented languages like C++ must consider many-to-many dependency relationships with inherent complexity, the Layered Component Model only needs to consider relationships with the layers one level above and one level below, making design, implementation, modification, and extension simpler.
A full-stack game engine is complex and large software, but by assembling it through one-to-one relationships of simple components, it becomes easier to build as a whole, resulting in improved portability.

The reason for improved portability is that the bottom layer is a "hardware abstraction layer" that absorbs OS differences, and the layers above it can be designed and implemented without any concern for OS differences.

```
+-----------------------------------+
| VN Engine (Suika3)                |
+-----------------------------------+
| 2D Game Engine (Playfield Engine) |
+-----------------------------------+
| Scripting Language (NoctLang)     |
+-----------------------------------+
| Hardware Abstraction Layer        |
| (StratoHAL)                       |
+-----------------------------------+
```
