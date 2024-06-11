# Arabiki64 - A Sample Sausage64 Model Parser

This folder contains a sample program that demonstrates how to parse the Sausage64 format and convert it to something else, such as a Nintendo64 Display List or Libdragon compatible OpenGL structures. The parser itself isn't fantastic, as it makes a lot of assumptions regarding how the s64 file is formatted. As long as you feed the tool something that was exported from Blender, it should be fine.

By default, models will be exported as a binary file, and a header file is generated with some helper macros. The program can also dump all the data into C structs if you prefer.

The program uses Forsyth's vertex cache optimization algorithm to fit the model in the vertex cache. The final mesh sorting could be further optimized to reduce display list commands. This is a sample tool, after all, you are free to use it as inspiration, or contribute to the repository to improve it!


### Usage
Executing the program without any arguments will display a full list of accepted inputs. The most basic usage of the program is as follows:

`arabiki64 -f model.s64`

The following optional arguments are accepted:

* `-t <File>` - A list of materials and their data. More information in the [materials section of the wiki](../../../wiki/4%29-Arabiki64%3A-Example-S64-to-Display-List-Converter#materials).
* `-s` - Export as C structs.
* `-g` - Export an OpenGL compatible model instead.
* `-2` - Disables 2tri optimization (required if using Fast3D) (Libultra only).
* `-c <Int>` - Change the size of the vertex cache. Default is `32` (Libultra only).
* `-i` - Omits the display list setup on the very first mesh load (in case you deem it unecessary) (Libultra only).
* `-n <Name>` - Sets the model name for the exported file. Default is `MyModel`.
* `-o <File>`- Sets the outputted display list's file name. Default is `outdlist.h`.
* `-q` - Quiet mode. Prevents the program from outputting info that you probably don't care about.
* `-r` - Disable the correction of the mesh's position data from the root coordinate.

**If you are using Libdragon as opposed to Libultra, you must use the `-g` flag.**


### Compiling
Compiling is very simple, as the program is entirely self contained and does not rely on external libraries.

If you are on Windows, assuming you have GCC installed and setup, you can compile by executing `makeme.bat`.

If you are on Linux or macOS, compilation can be done by just calling `make`.


### Using the Program
For more information on how to use Arabiki64, check out [the wiki](../../../wiki/4%29-Arabiki64%3A-Example-S64-to-Display-List-Converter).
