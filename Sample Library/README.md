# Sausage64 Sample Library

This library contains a single .c and .h file which you can drag and drop into your N64 project in order to have support for Sausage64 models converted by the [Sample Parser](../Sample%20Parser) tool. It is designed for both Libultra and Libdragon (using OpenGL).

In order to use the library with Libdragon, make sure you uncomment the `#define LIBDRAGON` to enable Libdragon support. The API changes slightly between both versions, so please double check below what functions you are supposed to be using.

The library is, currently, very basic. It can be expanded in the future in order to add more features, such as animation speed and blending. 

With this implementation of the library, matrix transformations are done on the CPU in order to reduce the memory footprint. This does mean that the CPU will be doing a bit more work, but that will probably not be too much of a problem given that most games are fillrate limited. Animations are also expected to playback at 30 frames per second.

A tutorial on how to use the library is available [in the wiki](../../../wiki/5%29-Sample-library-tutorial). You also have an example implementation available in the [Sample ROM](../Sample%20ROM) folder.

<details><summary>Included functions list (Libultra)</summary>
<p>
    
```c
/*==============================
    sausage64_initmodel
    Initialize a model helper struct
    @param The model helper to initialize
    @param The model data 
    @param An array of matrices for each mesh part
==============================*/
extern void sausage64_initmodel(s64ModelHelper* mdl, s64ModelData* mdldata, Mtx* matrices);

/*==============================
    sausage64_set_camera
    Sets the camera for Sausage64 to use for billboarding
    @param The view matrix
    @param The projection matrix
==============================*/
extern void sausage64_set_camera(Mtx* view, Mtx* projection);

/*==============================
    sausage64_set_anim
    Sets an animation on the model. Does not perform 
    error checking if an invalid animation is given.
    @param The model helper pointer
    @param The ANIMATION_* macro to set
==============================*/
extern void sausage64_set_anim(s64ModelHelper* mdl, u16 anim);
        
/*==============================
    sausage64_set_animcallback
    Set a function that gets called when an animation finishes
    @param The model helper pointer
    @param The animation end callback function
==============================*/
extern void sausage64_set_animcallback(s64ModelHelper* mdl, void (*animcallback)(u16));

/*==============================
    sausage64_set_predrawfunc
    Set a function that gets called before any mesh is rendered
    @param The model helper pointer
    @param The pre draw function
==============================*/
extern void sausage64_set_predrawfunc(s64ModelHelper* mdl, void (*predraw)(u16));

/*==============================
    sausage64_set_postdrawfunc
    Set a function that gets called after any mesh is rendered
    @param The model helper pointer
    @param The post draw function
==============================*/
extern void sausage64_set_postdrawfunc(s64ModelHelper* mdl, void (*postdraw)(u16));

/*==============================
    sausage64_advance_anim
    Advances the animation tick by the given amount
    @param The model helper pointer
    @param The amount to increase the animation tick by
==============================*/
extern void sausage64_advance_anim(s64ModelHelper* mdl, float tickamount);

/*==============================
    sausage64_drawmodel
    Renders a Sausage64 model
    @param A pointer to a display list pointer
    @param The model helper data
==============================*/
extern void sausage64_drawmodel(Gfx** glistp, s64ModelHelper* mdl);
```
</p>
</details>
</br>


<details><summary>Included functions list (Libdragon)</summary>
<p>
    
```c
/*==============================
    sausage64_initmodel
    Initialize a model helper struct
    @param The model helper to initialize
    @param The model data 
    @param An array of GL buffers for each mesh's verticies and faces
==============================*/
extern void sausage64_initmodel(s64ModelHelper* mdl, s64ModelData* mdldata, GLuint* glbuffers);

/*==============================
    sausage64_set_camera
    Sets the camera for Sausage64 to use for billboarding
    @param The view matrix
    @param The projection matrix
==============================*/
extern void sausage64_set_camera(Mtx* view, Mtx* projection);

/*==============================
    sausage64_set_anim
    Sets an animation on the model. Does not perform 
    error checking if an invalid animation is given.
    @param The model helper pointer
    @param The ANIMATION_* macro to set
==============================*/
extern void sausage64_set_anim(s64ModelHelper* mdl, u16 anim);
        
/*==============================
    sausage64_set_animcallback
    Set a function that gets called when an animation finishes
    @param The model helper pointer
    @param The animation end callback function
==============================*/
extern void sausage64_set_animcallback(s64ModelHelper* mdl, void (*animcallback)(u16));

/*==============================
    sausage64_set_predrawfunc
    Set a function that gets called before any mesh is rendered
    @param The model helper pointer
    @param The pre draw function
==============================*/
extern void sausage64_set_predrawfunc(s64ModelHelper* mdl, void (*predraw)(u16));

/*==============================
    sausage64_set_postdrawfunc
    Set a function that gets called after any mesh is rendered
    @param The model helper pointer
    @param The post draw function
==============================*/
extern void sausage64_set_postdrawfunc(s64ModelHelper* mdl, void (*postdraw)(u16));

/*==============================
    sausage64_advance_anim
    Advances the animation tick by the given amount
    @param The model helper pointer
    @param The amount to increase the animation tick by
==============================*/
extern void sausage64_advance_anim(s64ModelHelper* mdl, float tickamount);

/*==============================
    sausage64_loadmaterial
    Loads a material for libdragon rendering
    @param The material to load
==============================*/
extern void sausage64_loadmaterial(s64Material* mat);

/*==============================
    sausage64_drawmodel
    Renders a Sausage64 model
    @param The model helper data
==============================*/
extern void sausage64_drawmodel(s64ModelHelper* mdl);
```
</p>
</details>
</br>