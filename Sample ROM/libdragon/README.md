# Sample ROM

This folder contains a sample ROM for the Nintendo64, which demonstrates the exported model and animations (from the [Sample Model folder](../../Sample%20Model)) in action. 

**This ROM is not reading the S64 file directly**. Rather, the S64 file went through the `Sample Parser` program, which converted the model data to N64 display lists and the animation data to C structs. This outputted data can be found in `catherineMdl.h`. The face animation data and struct was custom made for this sample.


### Playing the ROM
The ROM should work on any official Nintendo64 hardware, and most emulators. 


### Compiling the ROM
Simply call `make`. A compiled version of the ROM should be available in the [releases page](../../../../releases).