
# Arabiki64 - A Sample Sausage64 Model Parser

This folder contains a sample program that demonstrates how to parse the Sausage64 format and convert it to something else, such as a Nintendo64 Display List. The parser itself isn't fantastic, as it makes a lot of assumptions regarding how the s64 file is formatted. As long as you feed the tool something that was exported from Blender, it should be fine.

The program uses Forsyth's vertex cache optimization algorithm to fit the model in the vertex cache. If it is unable to, it will error and request that you duplicate some vertices manually in Blender. This can be potentially automated in the future. The final mesh sorting could also be further optimized to reduce display list commands. This is a sample tool, after all, you are free to use it as inspiration, or contribute to the repository to improve it!

### Compiling

Compiling is very simple, as the program is entirely self contained and does not rely on external libraries.

If you are on Windows, assuming you have GCC installed and setup, you can compile by executing `makeme.bat`.

If you are on Linux or macOS, compilation can be done in one line:
```
gcc -O3 -o arabiki64 main.c datastructs.c mesh.c texture.c animation.c parser.c dlist.c output.c -lm
```

### Usage
Executing the program without any arguments will display a full list of accepted inputs. The most basic usage of the program is as follows:

`arabiki64 -f model.s64`

The following optional arguments are accepted:
* `-c <Int>` - Change the size of the vertex cache. Default is `32`.
* `-i` - Omits the display list setup on the very first mesh load (in case you deem it unecessary).
* `-n <Name>` - Sets the model name for the exported file. Default is `MyModel`.
* `-o <File>`- Sets the outputted display list's file name. Default is `outdlist.h`.
* `-q` - Quiet mode. Prevents the program from outputting info that you probably don't care about.
* `-r` - Disable the correction of the mesh's position data from the root coordinate.
* `-t <File>` - A list of textures and their data. More information in the next section.

### Textures

Because Nintendo 64 display lists require texture information, the program will request input from the user to fill in those details. This process can be automated by feeding the tool a text file with list of texture information. The syntax of the text file should be as follows:
```
NAME TYPE INFO FLAGS
```
Where:
* `NAME` should be the name of the texture/material. 
* `TYPE` should be the type of the texture. Accepted values are `TEXTURE`, `PRIMCOL`, or `OMIT`. Using the `OMIT` type will prevent Arabiki64 from generating display list commands for this texture.
* `INFO` should be information relevant to the texture. If the given type was `TEXTURE`, then the width and height of the texture in texels should be given (separated by spaces). If the given type was `PRIMCOL`, then RGB values should be provided (ranging from 0 to 255, separated by spaces).
* `FLAGS` are any relevant flags for this texture. You can change cycle type, combine modes, render modes, texture filters, image format (RGBA, Color Index, etc...), image pixel size, texture modes (like mirror and clamp), or geometry modes. All you need to do is to provide those macros, separated by spaces (please note that Arabiki does not validate whether the macro is valid or not). Example: `G_CC_2CYCLE G_RM_AA_ZB_TEX_EDGE G_RM_AA_ZB_TEX_EDGE2 G_IM_FMT_RGBA G_IM_SIZ_32b G_TX_CLAMP G_TX_CLAMP`.

You can use the `DONTLOAD` flag to prevent Arabiki64 from generating display list commands for **loading** the texture/primitive color, **but still generate the change of flags**. This is useful, for instance, if you have a dynamic texture.

The default flags for all textures are as follows:
```
Defualt Cycle: G_CYC_1CYCLE
Default Texture Filter: G_TF_BILERP
Default Combine Mode (For Textures): G_CC_MODULATEIDECALA G_CC_MODULATEIDECALA
Default Combine Mode (For Primitive Colors): G_CC_PRIMLITE G_CC_PRIMLITE
Default Render Mode: G_RM_AA_ZB_OPA_SURF G_RM_AA_ZB_OPA_SURF2
Default Texture Type: G_IM_FMT_RGBA
Default Texture Size: G_IM_SIZ_16b
Default Texture Modes: G_TX_MIRROR G_TX_MIRROR
Default Geometry Mode: G_SHADE | G_ZBUFFER | G_CULL_BACK | G_SHADING_SMOOTH
```

`G_CC_PRIMLITE` is a custom combine mode that allows for mixing of primitive colors with lighting. Arabiki64 will generate a macro for this in your display list file.

If you wish to remove one of the default geometry modes, you can do so by listing it in the flags with an exclamation mark. For instance, if you wanted to have a texture without backface culling, you'd provide the flag: `!G_CULL_BACK`.

Each texture should be separated by a new line. For an example texture list, check [sample/CatherineTextures.txt](sample/CatherineTextures.txt).
