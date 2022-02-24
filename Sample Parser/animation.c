/***************************************************************
                           animation.c
                             
Handles the parsing and creation of animations
***************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "main.h"
#include "texture.h"
#include "mesh.h"
#include "animation.h"


/*==============================
    add_animation
    Creates an animation object and adds it to the global list of animations
    @param The name of the animation
    @returns A pointer to the created animation
==============================*/

s64Anim* add_animation(char* name)
{
    // Allocate memory for the animation struct and string name
    s64Anim* anim = (s64Anim*)calloc(1, sizeof(s64Anim));
    if (anim == NULL)
        terminate("Error: Unable to allocate memory for animation object\n");
    anim->name = (char*)calloc(strlen(name)+1, 1);
    if (anim->name == NULL)
        terminate("Error: Unable to allocate memory for animation name\n");
    
    // Store the data in the newly created animation struct
    strcpy(anim->name, name);
    
    // Add this animation to our animation list and return it
    list_append(&list_animations, anim);
    return anim;
}


/*==============================
    add_keyframe
    Creates a keyframe object and adds it to an animation's list of keyframes
    @param A pointer to the animation
    @returns A pointer to the created keyframe
==============================*/

s64Keyframe* add_keyframe(s64Anim* anim, unsigned int keyframe)
{
    s64Keyframe* keyf = (s64Keyframe*)calloc(1, sizeof(s64Keyframe));
    if (keyf == NULL)
        terminate("Error: Unable to allocate memory for animation keyframe\n");
    keyf->keyframe = keyframe;
    list_append(&(anim->keyframes), keyf);
    return keyf;
}


/*==============================
    add_framedata
    Creates a framedata object and adds it to a keyframe's list of framedata
    @param A pointer to the keyframe
    @returns A pointer to the created framedata
==============================*/

s64FrameData* add_framedata(s64Keyframe* frame)
{
    s64FrameData* fdata = (s64FrameData*)calloc(1, sizeof(s64FrameData));
    if (fdata == NULL)
        terminate("Error: Unable to allocate memory for animation framedata\n");
    list_append(&(frame->framedata), fdata);
    return fdata;
}