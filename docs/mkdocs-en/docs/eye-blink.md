Eye Blinking
============

An eye blinking image must be stored in the "eye" folder that is
located in the same folder as the character's image(s).

- happy.png (main character file)
- eye/
    - happy.png (eye blinking file)

An eye blinking image consists of frame(s) of eye blinking difference(s). 
A frame must be the same size as the character's image is. 
Frames must be stored horizontally in the left to right order. See the sample game for the actual image.

Alpha values on borders must be smoothed.
Please use "Blur Selection" and "Delete Selection" in an image editing software.

The eye blinking interval may be specified in the `config.ini` file.
Intervals are slightly randomized, and sometimes double-blinkings occur.

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
