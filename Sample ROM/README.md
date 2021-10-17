# Sample ROM

This folder contains a sample ROM for the Nintendo64, which demonstrates the exported model and animations (from the [Sample Model folder](../Sample%20Model)) in action. 

**This ROM is not reading the S64 file directly**. Rather, the S64 file went through the `Sample Parser` program, which converted the model data to N64 display lists and the animation data to C structs. The textures were converted to 16-Bit RGBA format using [Buu342's N64 Texture Converter](https://github.com/buu342/GML-N64TextureConverter). This outputted data can be found in `catherineMdl.h` and `catherineTex.h`. The face animation data and struct was custom made for this sample.


### Playing the ROM
The ROM should work on any official Nintendo64 hardware, and most emulators. 

The controls are:
* Use the control stick to pan the camera.
* Hold R and use the control stick to rotate the camera.
* Hold Z and use the control stick to zoom the view.
* Press START to reset the camera position
* Press L to toggle the menu. You can select menu entries with the D pad, and toggle them with the A button.

The menu allows you to modify the model and face animations, toggle lighting, freeze the light position (which, by default, is always being projected from the camera), and to disable animation interpolation + looping.


### Compiling the ROM
Assuming you have the Libultra SDK installed in your machine, simply run `makeme.bat`. This currently only works on WindowsXP. A compiled version of the ROM should be available in the [releases page](../../../releases).


### USB
If you modify `debug.h` by changing `DEBUG_MODE` to `1`, you can enable USB input+output for flashcarts, allowing you to modify the ROM with your command prompt. This requires loading the ROM using a program such as [UNFLoader](https://github.com/buu342/N64-UNFLoader) with debug mode enabled.
