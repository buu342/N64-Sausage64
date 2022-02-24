#ifndef SAUSAGE64_H
#define SAUSAGE64_H

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
        Gfx* dl;
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
    
    extern void sausage64_drawmodel(Gfx** glistp, s64ModelHelper* mdl);

#endif