Lip Synchronization
===================

An lip synchronization image must be stored in the "lip" folder folder that is
located in the same folder as the character's image(s).

* happy.png (main character file)
* lip/
    * happy.png (lip synchronization file)

A lip synchronization image consists of frame(s) of lip synchronization difference(s).
A frame must be the same size as the character's image is.
Frames must be stored horizontally in the left to right order.

Alpha values on borders must be smoothed.
Please use "Blur Selection" and "Delete Selection" in an image editing software.

The lip synchronization interval may be specified in the `project.txt` file.

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
