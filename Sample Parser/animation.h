#ifndef _SAUSN64_ANIMATION_H
#define _SAUSN64_ANIMATION_H

    /*********************************
                 Structs
    *********************************/
    
    // Framedata struct
    typedef struct {
        s64Mesh* mesh;
        Vector3D translation;
        Vector4D rotation;
        Vector3D scale;
    } s64Transform;
    
    // Keyframe struct
    typedef struct {
        unsigned int keyframe;
        linkedList framedata;
    } s64Keyframe;

    // Animation struct
    typedef struct {
        char* name;
        linkedList keyframes;
    } s64Anim;
    
    
    /*********************************
                Functions
    *********************************/
    
    extern s64Anim*      add_animation(char* name);
    extern s64Keyframe*  add_keyframe(s64Anim* anim, unsigned int keyframe);
    extern s64Transform* add_framedata(s64Keyframe* frame);
    
#endif