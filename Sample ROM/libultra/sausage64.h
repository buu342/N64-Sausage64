#ifndef SAUSAGE64_H
#define SAUSAGE64_H

    // UNCOMMENT THE #DEFINE IF USING LIBDRAGON
    //#define LIBDRAGON
    
    // World space assumptions
    #define S64_UPVEC {0.0f, 0.0f, 1.0f}
    #define S64_FORWARDVEC {0.0f, -1.0f, 0.0f}


    /*********************************
      Libultra types (for libdragon)
    *********************************/

    #ifdef LIBDRAGON
        #include <stdint.h>
        #ifndef __GL_H__
            #include <GL/gl.h>
        #endif

        typedef uint8_t  u8;
        typedef uint16_t u16;
        typedef uint32_t u32;
        typedef uint64_t u64;

        typedef int8_t  s8;
        typedef int16_t s16;
        typedef int32_t s32;
        typedef int64_t s64;

        typedef volatile uint8_t  vu8;
        typedef volatile uint16_t vu16;
        typedef volatile uint32_t vu32;
        typedef volatile uint64_t vu64;

        typedef volatile int8_t  vs8;
        typedef volatile int16_t vs16;
        typedef volatile int32_t vs32;
        typedef volatile int64_t vs64;

        typedef float  f32;
        typedef double f64;
        typedef float  Mtx[4][4];
    #else
        #ifndef _ULTRA64_H_
            #include <ultra64.h>
        #endif
        #define s64Gfx Gfx
    #endif


    /*********************************
            Sausage64 Structs
    *********************************/

    #ifdef LIBDRAGON
        typedef enum {
            TYPE_OMIT = 0,
            TYPE_TEXTURE = 1,
            TYPE_PRIMCOL = 2
        } s64MaterialType;

        typedef struct {
            GLuint* identifier;
            u32 w;
            u32 h;
            GLuint filter;
            GLuint wraps;
            GLuint wrapt;
        } s64Texture;

        typedef struct {
            u8 r;
            u8 g;
            u8 b;
            u8 a;
        } s64PrimColor;

        typedef struct {
            s64MaterialType type;
            void* data;
            u8 lighting;
            u8 cullfront;
            u8 cullback;
            u8 smooth;
            u8 depthtest;
        } s64Material;

        typedef struct {
            f32 (*verts)[11];
            u16 vertcount;
            u16 facecount;
            u16 (*faces)[3];
            s64Material* material;
        } s64RenderBlock;

        typedef struct {
            u32 blockcount;
            GLuint guid_mdl;
            GLuint guid_verts;
            GLuint guid_faces;
            s64RenderBlock* renders;
        } s64Gfx;
    #endif
    
    typedef struct {
        f32 pos[3];
        f32 rot[4];
        f32 scale[3];
    } s64Transform;

    typedef struct {
        s64Transform data;
        u32 rendercount;
    } s64FrameTransform;

    typedef struct {
        const u32 framenumber;
        const s64Transform* framedata;
    } s64KeyFrame;

    typedef struct {
        const char* name;
        const u32 keyframecount;
        const s64KeyFrame* keyframes;
    } s64Animation;

    typedef struct {
        const char* name;
        const u32 is_billboard;
        const s64Gfx* dl;
        const s32 parent;
    } s64Mesh;

    typedef struct {
        const u16 meshcount;
        const u16 animcount;
        const s64Mesh* meshes;
        const s64Animation* anims;
        #ifndef LIBDRAGON
            Vtx* _vtxcleanup;
        #else
            s64Texture* _texcleanup;
            s64PrimColor* _primcolcleanup;
        #endif
    } s64ModelData;
    
    typedef struct {
        const s64Animation* animdata;
        f32 curtick;
        u32 curkeyframe;
    } s64AnimPlay;

    typedef struct {
        u8    interpolate;
        u8    loop;
        u32   rendercount;
        #ifndef LIBDRAGON
            Mtx* matrix;
        #endif
        u8    (*predraw)(u16);
        void  (*postdraw)(u16);
        void  (*animcallback)(u16);
        const s64ModelData* mdldata;
        s64FrameTransform* transforms; 
        s64AnimPlay curanim;
        s64AnimPlay blendanim;
        f32 blendticks;
        f32 blendticks_left;
    } s64ModelHelper;


    /*********************************
              Asset Loading
    *********************************/
    
    /*==============================
        sausage64_load_binarymodel
        Load a binary model from ROM
        @param  (Libultra) The starting address in ROM
        @param  (Libdragon) The dfs file path of the asset
        @param  (Libultra) The size of the model
        @param  (Libultra) The list of textures to use
        @param  (Libdragon) The list of dfs file paths of textures
        @return The newly allocated model
    ==============================*/

    #ifndef LIBDRAGON
        extern s64ModelData* sausage64_load_binarymodel(u32 romstart, u32 size, u32** textures);
    #else
        extern s64ModelData* sausage64_load_binarymodel(char* filepath, sprite_t** textures);        
    #endif


    /*==============================
        sausage64_unload_binarymodel
        Free the memory used by a dynamically loaded binary model
        @param  The model to free
    ==============================*/
    
    extern void sausage64_unload_binarymodel(s64ModelData* mdl);
    

    #ifdef LIBDRAGON
        /*==============================
            sausage64_load_texture
            Generates a texture for a static OpenGL model.
            Since the s64Texture struct contains a bunch of information,
            this function lets us create these textures with the correct
            attributes automatically.
            @param The Sausage64 texture
            @param The texture data itself, in a sprite struct
        ==============================*/

        extern void sausage64_load_texture(s64Texture* tex, sprite_t* texture);


        /*==============================
            sausage64_unload_texture
            Unloads a static texture created for OpenGL
            @param The s64 texture to unload
        ==============================*/

        extern void sausage64_unload_texture(s64Texture* tex);


        /*==============================
            sausage64_load_staticmodel
            Generates the display lists for a
            static OpenGL model
            @param The pointer to the model data
                   to generate
        ==============================*/
        
        extern void sausage64_load_staticmodel(s64ModelData* mdldata);


        /*==============================
            sausage64_load_staticmodel
            Frees the memory used by the display 
            lists of a static OpenGL model
            @param The pointer to the model data
                   to free
        ==============================*/

        extern void sausage64_unload_staticmodel(s64ModelData* mdldata);
    #endif

    
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
    
    extern s64ModelHelper* sausage64_inithelper(s64ModelData* mdldata);


    /*==============================
        sausage64_freehelper
        Frees the memory used up by a Sausage64 model helper
        @param A pointer to the model helper
    ==============================*/

    extern void sausage64_freehelper(s64ModelHelper* helper);
    

    #ifdef LIBDRAGON
         /*==============================
            sausage64_loadmaterial
            Loads a material for libdragon rendering
            @param The material to load
        ==============================*/
    
        extern void sausage64_loadmaterial(s64Material* mat);
    #endif


    /*==============================
        sausage64_set_camera
        Sets the camera for Sausage64 to use for billboarding
        @param (Libultra) The view matrix
        @param (Libultra) The projection matrix
        @param (Libdragon) The location of the camera, relative to the model's root
    ==============================*/
    
    #ifndef LIBDRAGON
        extern void sausage64_set_camera(Mtx* view, Mtx* projection);
    #else
        extern void sausage64_set_camera(f32 campos[3]);
    #endif

    
    /*==============================
        sausage64_set_anim
        Sets an animation on the model. Does not perform 
        error checking if an invalid animation is given.
        @param The model helper pointer
        @param The ANIMATION_* macro to set
    ==============================*/
    
    extern void sausage64_set_anim(s64ModelHelper* mdl, u16 anim);

    
    /*==============================
        sausage64_set_anim_blend
        Sets an animation on the model with blending. Does not perform 
        error checking if an invalid animation is given.
        @param The model helper pointer
        @param The ANIMATION_* macro to set
        @param The amount of ticks to blend the animation over
    ==============================*/
    
    extern void sausage64_set_anim_blend(s64ModelHelper* mdl, u16 anim, f32 ticks);
    
    
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
    
    extern void sausage64_set_predrawfunc(s64ModelHelper* mdl, u8 (*predraw)(u16));
    
    
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
    
    extern void sausage64_advance_anim(s64ModelHelper* mdl, f32 tickamount);
    
    
    /*==============================
        sausage64_get_meshtransform
        Get the current transform of the mesh,
        local to the object
        @param  The model helper pointer
        @param  The mesh to check
        @return The mesh's local transform
    ==============================*/
    
    extern s64Transform* sausage64_get_meshtransform(s64ModelHelper* mdl, const u16 mesh);
    
    
    /*==============================
        sausage64_lookat
        Make a mesh look at another
        @param The model helper pointer
        @param The mesh to force the lookat
        @param The normalized direction vector
        @param A value from 1.0 to 0.0 stating how much to look at the object
        @param Whether the lookat should propagate to the children meshes (up to one level)
    ==============================*/
    
    extern void sausage64_lookat(s64ModelHelper* mdl, const u16 mesh, f32 dir[3], f32 amount, u8 affectchildren);


    /*==============================
        sausage64_drawmodel
        Renders a Sausage64 model
        @param (Libultra) A pointer to a display list pointer
               (Libdragon) The model helper data
        @param (Libultra) The model helper data
    ==============================*/
    
    #ifndef LIBDRAGON
        extern void sausage64_drawmodel(Gfx** glistp, s64ModelHelper* mdl);
    #else
        extern void sausage64_drawmodel(s64ModelHelper* mdl);
    #endif

#endif