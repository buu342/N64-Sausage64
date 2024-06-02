# Sausage64 Sample Library

This library contains a single .c and .h file which you can drag and drop into your N64 project in order to have support for Sausage64 models converted by the [Sample Parser](../Sample%20Parser) tool. It is designed for both Libultra and Libdragon (using OpenGL).

Because the library uses `malloc` internally, Libultra projects are expected to have the heap set up properly using `InitHeap`. Failure to do so will likely result in a crash at startup.

In order to use the library with Libdragon, make sure you uncomment the `#define LIBDRAGON` to enable Libdragon support. The API changes slightly between both versions, so please double check below what functions you are supposed to be using.

With this implementation of the library, matrix transformations are done on the CPU in order to reduce the memory footprint. This does mean that the CPU will be doing a bit more work, but that will probably not be too much of a problem given that most games are fillrate limited. Animations are also expected to playback at 30 frames per second.

A tutorial on how to use the library is available [in the wiki](../../../wiki/5%29-Sample-library-tutorial). You also have an example implementation available in the [Sample ROM](../Sample%20ROM) folder.

<details><summary>Included functions list (Libultra)</summary>
<p>
    
```c
/*==============================
    sausage64_inithelper
    Allocate a new model helper struct
    @param  The model data
    @return A newly allocated model helper, or
            NULL if it failed to allocate
==============================*/
s64ModelHelper* sausage64_inithelper(s64ModelData* mdldata);

/*==============================
    sausage64_set_camera
    Sets the camera for Sausage64 to use for billboarding
    @param The view matrix
    @param The projection matrix
==============================*/
void sausage64_set_camera(Mtx* view, Mtx* projection);

/*==============================
    sausage64_set_anim
    Sets an animation on the model. Does not perform 
    error checking if an invalid animation is given.
    @param The model helper pointer
    @param The ANIMATION_* macro to set
==============================*/
void sausage64_set_anim(s64ModelHelper* mdl, u16 anim);

/*==============================
    sausage64_set_anim_blend
    Sets an animation on the model with blending. Does not perform 
    error checking if an invalid animation is given.
    @param The model helper pointer
    @param The ANIMATION_* macro to set
    @param The amount of ticks to blend the animation over
==============================*/
void sausage64_set_anim_blend(s64ModelHelper* mdl, u16 anim, f32 ticks);

/*==============================
    sausage64_set_animcallback
    Set a function that gets called when an animation finishes
    @param The model helper pointer
    @param The animation end callback function
==============================*/
void sausage64_set_animcallback(s64ModelHelper* mdl, void (*animcallback)(u16));

/*==============================
    sausage64_set_predrawfunc
    Set a function that gets called before any mesh is rendered
    @param The model helper pointer
    @param The pre draw function
==============================*/
void sausage64_set_predrawfunc(s64ModelHelper* mdl, u8 (*predraw)(u16));

/*==============================
    sausage64_set_postdrawfunc
    Set a function that gets called after any mesh is rendered
    @param The model helper pointer
    @param The post draw function
==============================*/
void sausage64_set_postdrawfunc(s64ModelHelper* mdl, void (*postdraw)(u16));

/*==============================
    sausage64_advance_anim
    Advances the animation tick by the given amount
    @param The model helper pointer
    @param The amount to increase the animation tick by
==============================*/
void sausage64_advance_anim(s64ModelHelper* mdl, f32 tickamount);

/*==============================
    sausage64_get_meshtransform
    Get the current transform of the mesh,
    local to the object
    @param  The model helper pointer
    @param  The mesh to check
    @return The mesh's local transform
==============================*/
s64Transform* sausage64_get_meshtransform(s64ModelHelper* mdl, const u16 mesh);

/*==============================
    sausage64_lookat
    Make a mesh look at another
    @param The model helper pointer
    @param The mesh to force the lookat
    @param The normalized direction vector
    @param A value from 1.0 to 0.0 stating how much to look at the object
    @param Whether the lookat should propagate to the children meshes (up to one level)
==============================*/
void sausage64_lookat(s64ModelHelper* mdl, const u16 mesh, f32 dir[3], f32 amount, u8 affectchildren);

/*==============================
    sausage64_drawmodel
    Renders a Sausage64 model
    @param A pointer to a display list pointer
    @param The model helper data
==============================*/
void sausage64_drawmodel(Gfx** glistp, s64ModelHelper* mdl);

/*==============================
    sausage64_freehelper
    Frees the memory used up by a Sausage64 model helper
    @param A pointer to the model helper
==============================*/
void sausage64_freehelper(s64ModelHelper* helper);
```
</p>
</details>
</br>


<details><summary>Included functions list (Libdragon)</summary>
<p>
    
```c
/*********************************
          Asset Loading
*********************************/

/*==============================
    sausage64_load_texture
    Generates a texture for OpenGL.
    Since the s64Texture struct contains a bunch of information,
    this function lets us create these textures with the correct
    attributes automatically.
    @param The Sausage64 texture
    @param The texture data itself, in a sprite struct
==============================*/
void sausage64_load_texture(s64Texture* tex, sprite_t* texture);

/*==============================
    sausage64_unload_texture
    Unloads a texture created for OpenGL
    @param The s64 texture to unload
==============================*/
void sausage64_unload_texture(s64Texture* tex);

/*==============================
    sausage64_load_staticmodel
    Generates the display lists for a
    static OpenGL model
    @param The pointer to the model data
           to generate
==============================*/
void sausage64_load_staticmodel(s64ModelData* mdldata);

/*==============================
    sausage64_load_staticmodel
    Frees the memory used by the display 
    lists of a static OpenGL model
    @param The pointer to the model data
           to free
==============================*/
void sausage64_unload_staticmodel(s64ModelData* mdldata);


/*********************************
       Sausage64 Functions
*********************************/

/*==============================
    sausage64_inithelper
    Allocate a new model helper struct
    @param  The model data
    @return A newly allocated model helper, or
            NULL if it failed to allocate
==============================*/
s64ModelHelper* sausage64_inithelper(s64ModelData* mdldata);

 /*==============================
    sausage64_loadmaterial
    Loads a material for libdragon rendering
    @param The material to load
==============================*/
void sausage64_loadmaterial(s64Material* mat);

/*==============================
    sausage64_set_camera
    Sets the camera for Sausage64 to use for billboarding
    @param The location of the camera, relative to the model's root
==============================*/
void sausage64_set_camera(f32 campos[3]);

/*==============================
    sausage64_set_anim
    Sets an animation on the model. Does not perform 
    error checking if an invalid animation is given.
    @param The model helper pointer
    @param The ANIMATION_* macro to set
==============================*/
void sausage64_set_anim(s64ModelHelper* mdl, u16 anim);

/*==============================
    sausage64_set_anim_blend
    Sets an animation on the model with blending. Does not perform 
    error checking if an invalid animation is given.
    @param The model helper pointer
    @param The ANIMATION_* macro to set
    @param The amount of ticks to blend the animation over
==============================*/
void sausage64_set_anim_blend(s64ModelHelper* mdl, u16 anim, f32 ticks);

/*==============================
    sausage64_set_animcallback
    Set a function that gets called when an animation finishes
    @param The model helper pointer
    @param The animation end callback function
==============================*/
void sausage64_set_animcallback(s64ModelHelper* mdl, void (*animcallback)(u16));

/*==============================
    sausage64_set_predrawfunc
    Set a function that gets called before any mesh is rendered
    @param The model helper pointer
    @param The pre draw function
==============================*/
void sausage64_set_predrawfunc(s64ModelHelper* mdl, u8 (*predraw)(u16));

/*==============================
    sausage64_set_postdrawfunc
    Set a function that gets called after any mesh is rendered
    @param The model helper pointer
    @param The post draw function
==============================*/
void sausage64_set_postdrawfunc(s64ModelHelper* mdl, void (*postdraw)(u16));

/*==============================
    sausage64_advance_anim
    Advances the animation tick by the given amount
    @param The model helper pointer
    @param The amount to increase the animation tick by
==============================*/
void sausage64_advance_anim(s64ModelHelper* mdl, f32 tickamount);

/*==============================
    sausage64_get_meshtransform
    Get the current transform of the mesh,
    local to the object
    @param  The model helper pointer
    @param  The mesh to check
    @return The mesh's local transform
==============================*/
s64Transform* sausage64_get_meshtransform(s64ModelHelper* mdl, const u16 mesh);

/*==============================
    sausage64_lookat
    Make a mesh look at another
    @param The model helper pointer
    @param The mesh to force the lookat
    @param The normalized direction vector
    @param A value from 1.0 to 0.0 stating how much to look at the object
    @param Whether the lookat should propagate to the children meshes (up to one level)
==============================*/
void sausage64_lookat(s64ModelHelper* mdl, const u16 mesh, f32 dir[3], f32 amount, u8 affectchildren);

/*==============================
    sausage64_drawmodel
    Renders a Sausage64 model
    @param A pointer to a display list pointer
           The model helper data
    @param The model helper data
==============================*/
void sausage64_drawmodel(s64ModelHelper* mdl);

/*==============================
    sausage64_freehelper
    Frees the memory used up by a Sausage64 model helper
    @param A pointer to the model helper
==============================*/
void sausage64_freehelper(s64ModelHelper* helper);
```
</p>
</details>
</br>