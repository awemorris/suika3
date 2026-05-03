
## Boot and Execution Sequence

<img src="sequence.svg">

- StratoHAL: `main()` is called.
- StratoHAL: Mount the package file.
- StratoHAL: Calls `hal_callback_on_event_setup()` in `Playfield Engine` tp determine the screen size.
    - Playfield Engine: `hal_callback_on_event_setup()` is called.
    - Playfield Engine: Loads `main.ray`.
    - Playfield Engine: Calls `setup()` in the `main.ray`.
        - Suika3 Game: `setup()` is called.
        - Suika3 Game: Returns the screen size.
    - Playfield Engine: Returns the screen size.
- StratoHAL: Now knows the screen size.
- StratoHAL: Creates the window.
- StratoHAL: Sets up audio.
- StratoHAL: Calls `hal_callback_on_event_start()` in `Playfield Engine`.
    - Playfield Engine: `hal_callback_on_event_start()` is called.
    - Playfield Engine: Calls `start()` in the `main.ray`.
        - Suika3 Game: `start()` is called.
        - Suika3 Game: Calls `Suika.start()`.
            - Suika3 Internal: `Suika.start()` is called.
            - Suika3 Internal: Load `config.ini`.
            - Suika3 Internal: Load `start.novel`.
            - Suika3 Internal: Returns.
        - Suika3 Game: Returns.
    - Playfield Engine: Returns.
- StratoHAL: Game Loop:
    - StratoHAL: Calls `on_hal_callback_update()` in `Playfield Engine`.
        - Playfield Engine: `on_hal_callback_update()` is called.
		- Playfield Engine: Calls `update()` in the `main.ray`.
            - Suika3 Game: `update()` is called.
            - Suika3 Game: Calls `Suika.update()`.
                - Suika3 Internal: `Suika.update()` is called.
                - Suika3 Internal: Get the current tag and its parameters.
                - Suika3 Internal: Calls `Tag_*()` correnponding to the current tag.
                    - Tag_*: Run a frame.
                    - Tag_*: Moves to the next tag if finished.
                    - Tag_*: Returns.
            - Suika3 Game: Returns.
		- Playfield Engine: Calls `render()` in the `main.ray`.
            - Suika3: `render()` is called.
            - Suika3: Calls `Suika.render()`.
                - Suika3 Internal: Renders the stage layers.
                - Suika3 Internal: Returns.
            - User Game: Returns.
    - StratoHAL: Finish rendering.
    - StratoHAL: Go back to the beginning of the loop.
