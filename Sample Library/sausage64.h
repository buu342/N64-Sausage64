#ifndef SAUSAGE64_H
#define SAUSAGE64_H

    // UNCOMMENT THE #DEFINE IF USING LIBDRAGON
    //#define LIBDRAGON


    /*********************************
      Libultra types (for libdragon)
    *********************************/

    #ifdef LIBDRAGON
        #include <stdint.h>
        #include <math.h>
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
        typedef float Mtx[4][4];
    #else
        #ifndef _ULTRA64_H_
            #include <ultra64.h>
        #endif
    #endif


    /*********************************
            Sausage64 Structs
    *********************************/

    typedef struct {
        f32 pos[3];
        f32 rot[4];
        f32 scale[3];
    } s64FrameData;

    typedef struct {
        u32 framenumber;
        s64FrameData* framedata;
    } s64KeyFrame;

    typedef struct {
        const char* name;
        u32 keyframecount;
        s64KeyFrame* keyframes;
    } s64Animation;

    typedef struct {
        const char* name;
        const u32 is_billboard;
        #ifndef LIBDRAGON
            Gfx* dl;
        #endif
    } s64Mesh;

    typedef struct {
        u16 meshcount;
        u16 animcount;
        s64Mesh* meshes;
        s64Animation* anims;
    } s64ModelData;

    typedef struct {
        u8 interpolate;
        u8 loop;
        s64Animation* curanim;
        u32 curanimlen;
        float animtick;
        u32 curkeyframe;
        Mtx* matrix;
        void (*predraw)(u16);
        void (*postdraw)(u16);
        void (*animcallback)(u16);
        s64ModelData* mdldata;
    } s64ModelHelper;
    
    
    /*********************************
           Sausage64 Functions
    *********************************/
    
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
    
    #ifndef LIBDRAGON
        extern void sausage64_drawmodel(Gfx** glistp, s64ModelHelper* mdl);
    #else
        extern void sausage64_drawmodel(s64ModelHelper* mdl);
    #endif

#endif