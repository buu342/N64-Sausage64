/***************************************************************
                          sausage64.c
                               
A very library that handles Sausage64 models exported by 
Arabiki64.
https://github.com/buu342/Blender-Sausage64
***************************************************************/

#include <ultra64.h>
#include "sausage64.h"


/*==============================
    sausage64_initmodel
    Initialize a model helper struct
    @param The model helper to initialize
    @param The model data 
    @param An array of matrices for each mesh part
==============================*/

void sausage64_initmodel(s64ModelHelper* mdl, s64ModelData* mdldata, Mtx* matrices)
{
    mdl->interpolate = TRUE;
    mdl->loop = TRUE;
    mdl->curkeyframe = 0;
    if (mdldata->animcount > 0)
    {
        s64Animation* animdata = &mdldata->anims[0];
        mdl->curanim = animdata;
        mdl->curanimlen = animdata->keyframes[animdata->keyframecount-1].framenumber;
    }
    else
    {
        mdl->curanim = NULL;
        mdl->curanimlen = 0;
    }
    mdl->animtick = 0;
    mdl->matrix = matrices;
    mdl->predraw = NULL;
    mdl->postdraw = NULL;
    mdl->animcallback = NULL;
    mdl->mdldata = mdldata;
}


/*==============================
    sausage64_set_predrawfunc
    Set a function that gets called before any mesh is rendered
    @param The model helper pointer
    @param The pre draw function
==============================*/

inline void sausage64_set_predrawfunc(s64ModelHelper* mdl, void (*predraw)(u16))
{
    mdl->predraw = predraw;
}


/*==============================
    sausage64_set_postdrawfunc
    Set a function that gets called after any mesh is rendered
    @param The model helper pointer
    @param The post draw function
==============================*/

inline void sausage64_set_postdrawfunc(s64ModelHelper* mdl, void (*postdraw)(u16))
{
    mdl->postdraw = postdraw;
}


/*==============================
    sausage64_set_animcallback
    Set a function that gets called when an animation finishes
    @param The model helper pointer
    @param The animation end callback function
==============================*/

inline void sausage64_set_animcallback(s64ModelHelper* mdl, void (*animcallback)(u16))
{
    mdl->animcallback = animcallback;
}


/*==============================
    sausage64_update_anim
    Updates the animation keyframe based on the animation 
    tick
    @param The model helper pointer
==============================*/

static void sausage64_update_anim(s64ModelHelper* mdl)
{
    const s64Animation* anim = mdl->curanim;
    const float curtick = mdl->animtick;
    const u32 curkeyframe = mdl->curkeyframe;
    const u32 curframenum = anim->keyframes[curkeyframe].framenumber;
    const u32 nframes = anim->keyframecount;
    const u32 animlen = mdl->curanimlen;
    u32 nextkeyframe = (curkeyframe+1)%nframes;
    
    // Check if we changed animation frame
    if (curtick >= anim->keyframes[nextkeyframe].framenumber || curtick < curframenum)
    {
        if (curtick > curframenum) // Animation advanced to next frame
        {
            u32 i=0;
            
            // Cycle through all frames, starting at the one after the current frame
            do
            {
                // Update the keyframe if we've passed this keyframe's number
                if (curtick >= anim->keyframes[nextkeyframe].framenumber)
                {
                    // Go to the next keyframe, and stop
                    mdl->curkeyframe = nextkeyframe;
                    return;
                }
                
                // If that was a failure, go to the next frame
                nextkeyframe = (nextkeyframe+1)%nframes;
                i++;
            }
            while (i<nframes);
        }
        else if (curkeyframe > 0 && curtick < anim->keyframes[1].framenumber) // Animation rolled over to the first frame (special case for speedup reasons)
        {
            mdl->curkeyframe = 0;
        }
        else // Animation is potentially going backwards
        {
            u32 i=0;
            s32 prevkeyframe = curkeyframe-1;
            if (prevkeyframe < 0)
                prevkeyframe = nframes-1;
            
            // Cycle through all frames (backwards), starting at the one before the current frame
            do
            {
                // Update the keyframe if we've passed this keyframe's number
                if (curtick >= anim->keyframes[prevkeyframe].framenumber)
                {
                    // Go to the next keyframe, and stop
                    mdl->curkeyframe = prevkeyframe;
                    return;
                }
                
                // If that was a failure, go to the previous frame
                prevkeyframe--;
                if (prevkeyframe < 0)
                    prevkeyframe = nframes-1;
                i++;
            }
            while (i<nframes);    
        }
    }
}


/*==============================
    sausage64_advance_anim
    Advances the animation tick by the given amount
    @param The model helper pointer
    @param The amount to increase the animation tick by
==============================*/

void sausage64_advance_anim(s64ModelHelper* mdl, float tickamount)
{
    char loop = TRUE;
    float division;
    mdl->animtick += tickamount;
    
    // If the animation ended, call the callback function and roll the tick value over
    if (mdl->animtick >= mdl->curanimlen)
    {
        // Execute the animation end callback function
        if (mdl->animcallback != NULL)
            mdl->animcallback(mdl->curanim - &mdl->mdldata->anims[0]);
            
        // If looping is disabled, then stop
        if (!mdl->loop)
        {
            mdl->animtick = mdl->curanimlen;
            mdl->curkeyframe = mdl->curanim->keyframecount-1;
            return;
        }
        
        // Calculate the correct tick         
        division = mdl->animtick/((float)mdl->curanimlen);
        mdl->animtick = (division - ((int)division))*((float)mdl->curanimlen);
    }
    else if (mdl->animtick <= 0)
    {
        // Execute the animation end callback function
        if (mdl->animcallback != NULL)
            mdl->animcallback(mdl->curanim - &mdl->mdldata->anims[0]);
            
        // If looping is disabled, then stop
        if (!mdl->loop)
        {
            mdl->animtick = 0;
            mdl->curkeyframe = 0;
            return;
        }
        
        // Calculate the correct tick       
        division = mdl->animtick/((float)mdl->curanimlen);
        mdl->animtick = (1+(division - ((int)division)))*((float)mdl->curanimlen);
        mdl->curkeyframe = mdl->curanim->keyframecount-1;
    }
    
    // Update the animation
    if (mdl->curanim->keyframecount > 0 && loop)
        sausage64_update_anim(mdl);
}


/*==============================
    sausage64_set_anim
    Sets an animation on the model. Does not perform 
    error checking if an invalid animation is given.
    @param The model helper pointer
    @param The ANIMATION_* macro to set
==============================*/

void sausage64_set_anim(s64ModelHelper* mdl, u16 anim)
{
    s64Animation* animdata = &mdl->mdldata->anims[anim];
    mdl->curanim = animdata;
    mdl->curanimlen = animdata->keyframes[animdata->keyframecount-1].framenumber;
    mdl->curkeyframe = 0;
    mdl->animtick = 0;
    if (animdata->keyframecount > 0)
        sausage64_update_anim(mdl);
}


/*==============================
    s64lerp
    Returns the linear interpolation of 
    two values given a fraction
    @param The first value
    @param The target value
    @param The fraction
==============================*/

static inline f32 s64lerp(f32 a, f32 b, f32 f)
{
    return a + f*(b - a);
}


/*==============================
    sausage64_drawpart
    Renders a part of a Sausage64 model
    @param A pointer to a display list pointer
    @param The current framedata
    @param The next framedata
    @param Whether to interpolate or not
    @param The interpolation amount
    @param The matrix to store the mesh's transformation
    @param The mesh's display list
==============================*/

static inline void sausage64_drawpart(Gfx** glistp, const s64FrameData* cfdata, const s64FrameData* nfdata, const u8 interpolate, const f32 l, Mtx* matrix, Gfx* dl)
{
    Mtx helper;

    // Initialize the matricies
    guMtxIdent(matrix);
    guMtxIdent(&helper);
    
    // Calculate the transformations on the CPU
    if (interpolate)
    {
        guTranslate(matrix, 
            s64lerp(cfdata->pos[0], nfdata->pos[0], l), 
            s64lerp(cfdata->pos[1], nfdata->pos[1], l), 
            s64lerp(cfdata->pos[2], nfdata->pos[2], l)
        );
        guScale(&helper, 
            s64lerp(cfdata->scale[0], nfdata->scale[0], l), 
            s64lerp(cfdata->scale[1], nfdata->scale[1], l), 
            s64lerp(cfdata->scale[2], nfdata->scale[2], l)
        );
        guMtxCatL(&helper, matrix, matrix);
        guRotateRPY(&helper,
            s64lerp(cfdata->rot[0], nfdata->rot[0], l), 
            s64lerp(cfdata->rot[1], nfdata->rot[1], l),
            s64lerp(cfdata->rot[2], nfdata->rot[2], l)
        );
        guMtxCatL(&helper, matrix, matrix);
    }
    else
    {
        guTranslate(matrix, cfdata->pos[0], cfdata->pos[1], cfdata->pos[2]);    
        guScale(&helper, cfdata->scale[0], cfdata->scale[1], cfdata->scale[2]);
        guMtxCatL(&helper, matrix, matrix);
        guRotateRPY(&helper, cfdata->rot[0], cfdata->rot[1], cfdata->rot[2]);
        guMtxCatL(&helper, matrix, matrix);
    }
    
    // Draw the body part
    gSPMatrix((*glistp)++, OS_K0_TO_PHYSICAL(matrix), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPDisplayList((*glistp)++, dl);
    gSPPopMatrix((*glistp)++, G_MTX_MODELVIEW);
}


/*==============================
    sausage64_drawmodel
    Renders a Sausage64 model
    @param A pointer to a display list pointer
    @param The model helper data
==============================*/

void sausage64_drawmodel(Gfx** glistp, s64ModelHelper* mdl)
{
    u16 i;
    const s64ModelData* mdata = mdl->mdldata;
    const u16 mcount = mdata->meshcount;
    const s64Animation* anim = mdl->curanim;
    
    // If we have a valid animation
    if (anim != NULL)
    {
        const s64KeyFrame* ckframe = &anim->keyframes[mdl->curkeyframe];
        const s64KeyFrame* nkframe = &anim->keyframes[(mdl->curkeyframe+1)%anim->keyframecount];
        f32 l = 0;
        
        // Prevent division by zero when calculating the lerp amount
        if (nkframe->framenumber - ckframe->framenumber != 0)
            l = ((f32)(mdl->animtick - ckframe->framenumber))/((f32)(nkframe->framenumber - ckframe->framenumber));
            
        // Iterate through each mesh
        for (i=0; i<mcount; i++)
        {
            const s64FrameData* cfdata = &ckframe->framedata[i];
            const s64FrameData* nfdata = &nkframe->framedata[i];
            
            // Call the pre draw function
            if (mdl->predraw != NULL)
                mdl->predraw(i);
                
            // Draw this part of the model with animations
            sausage64_drawpart(glistp, cfdata, nfdata, mdl->interpolate, l, &mdl->matrix[i],  mdata->meshes[i].dl);
            
            // Call the post draw function
            if (mdl->postdraw != NULL)
                mdl->postdraw(i);
        }
    }
    else
    {
        // Iterate through each mesh
        for (i=0; i<mcount; i++)
        {
            // Call the pre draw function
            if (mdl->predraw != NULL)
                mdl->predraw(i);
                
            // Draw this part of the model without animations
            gSPDisplayList((*glistp)++, mdata->meshes[i].dl);
            
            // Call the post draw function
            if (mdl->postdraw != NULL)
                mdl->postdraw(i);
        }
    }
}