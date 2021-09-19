# Sausage64

<img src=".github/Catherine.gif" width="179" height="281"/>

Sausage64 is a plugin for Blender 2.7 onwards, which allows you to export "sausage link" style character models with animations. 

The plugin exports the data as an easily parsable text file. This file should be easy to convert to other formats, more suitable for your target platform. 

This repository contains three folders:
* `Plugin` - The Blender plugin itself. Installation instructions provided in a further section of this README. 
* `Sample Model` - An example character model with animations, with the source files available and the .s64 file exported by the plugin. The Blender file is for 2.7 but can be opened on newer versions, at the expense of the materials looking different. 
* `Sample ROM` - An example N64 ROM displaying the character and the animations in action. The .s64 file and the textures were converted to display lists and other parsable data with a custom tool (which is currently not in this repository). More information available in the folder's README. 

### What do you mean by Sausage Links?
Sausage link characters are made out of different unconnected segments. 

<img src=".github/C29E784C-ABFE-493E-A042-FC4DFDF86F3D.jpeg" width="340" height="428"/>
Photo taken from: https://bradh2002.wordpress.com/a2-task-3-video-game-history/
</br></br>

This method of character design was highly popular in the N64 and PS1 era, where animating vertex skinned characters was considered a very heavy operation. Instead, these models leverage the fact that each segment can easily be transformed via a matrix.

### Installation 
1. Grab the python file that is inside the `Plugin` folder.
2. Open Blender, and go to your user preferences
3. Go to the addons tab, and select "Install External Script"
4. Find the python script in the file browser and open it. 
5. The script `Sausage64 Character Export` should appear. Tick the checkbox to enable it. 
6. If you are on Blender 2.8 onwards, you're all set! If you are on an earlier version of Blender, you will get a warning that the script is for a newer version of Blender. You can safely ignore this warning. Don't forget to press the `Save User Preferences` button. 

### S64 file format, Usage Instructions, FAQ, and More
For more information, please check the [wiki](../../wiki).
