# Chorizo - A Sample Sausage64 Model Viewer

<img src="../.github/Chorizo.png" width="800" height="430"/>

This folder contains a cross platform sample program that lets you view Sausage64 models, and to create material definitions for Arabiki in a visual manner.

### System Requirements
<details><summary>Windows</summary>
<p>
    
* Windows XP or higher.
* An OpenGL capable device.
</p>
</details>

<details><summary>Linux</summary>
<p>
    
* Ubuntu (Haven't tested with others)
* An OpenGL capable device.
</p>
</details>


### Compiling

This program uses [wxWidgets](https://www.wxwidgets.org/) to allow for a cross platform GUI. 

<details><summary>Windows</summary>
<p>

**If you cloned this repo, make sure that the GLM submodule was properly downloaded. Check the `Include` folder. If it does not have GLM inside, you must run `git submodule update --init --recursive`, or download GLM from [here](https://github.com/g-truc/glm) and place it there manually.**
    
Because Chorizo is designed with Windows XP compatibility in mind, I use Visual Studio 2019 with the old XP SDK (`v141_xp`) to compile the solution. If you are not interested in supporting XP, then feel free to use a later version of the IDE and SDK. 

Download the [wxWidgets source code ZIP](https://wxwidgets.org/downloads/), and extract it. If you are targeting Windows XP, use wxWidgets 3.2.8, otherwise use the latest. Go to the `build/msw` folder and open the solution file for your Visual Studio version. Since I am using VS 2019, open `wx_vc16.sln` (as Visual Studio 2019 refers to version 16). Once the solution loads, you will need to edit all the projects options to use `/MT` for code generation, as wxWidgets compiles with `/MD` by default. Afterwards, if you want XP compatibility then make sure you set the Platform Toolset to `v141_xp`. After those steps are completed, compile the wxWidgets static libraries using your preferred setup (debug/release 32/64). Once wxWidgets is finished compiling, make sure that the `WXWIN` environment variable is set on your system to point to the wxWidgets directory (if it isn't, do so). If you are able to compile the OpenGL samples (after changing them to `/MT` and their platform toolkit if needed), then you are good to go.

If you have successfully installed wxWidgets, then simply open `Chorizo.vcxproj` with Visual Studio to compile. If you want XP compatibility, then you will need to use the `Win32 (XP)` solution platform. Otherwise, use `x64`.

</p>
</details>

<details><summary>Linux</summary>
<p>

**If you cloned this repo, make sure that the GLM submodule was properly downloaded. Check the `Include` folder. If it does not have GLM inside, you must run `git submodule update --init --recursive`, or download GLM from [here](https://github.com/g-truc/glm) and place it there manually.**

Start by installing or building wxWidgets using this [guide](https://docs.wxwidgets.org/trunk/plat_gtk_install.html). **Make sure you compile with OpenGL enabled.** If you are able to compile the OpenGL samples (located in wxWidgets/samples/opengl), then you have succeeded.

If you have successfully installed wxWidgets, then simply run `make` to compile.

</details>