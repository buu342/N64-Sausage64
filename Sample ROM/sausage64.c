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
    mdl->curframeindex = 0;
    mdl->curanim = &mdldata->anims[0];
    mdl->animtick = 0;
    mdl->matrix = matrices;
    mdl->predraw = NULL;
    mdl->postdraw = NULL;
    mdl->mdldata = mdldata;
    mdl->nexttick = osGetTime()+OS_NSEC_TO_CYCLES(33333333);
}

inline void sausage64_set_predrawfunc(s64ModelHelper* mdl, void (*predraw)(u16))
{
    mdl->predraw = predraw;
}

inline void sausage64_set_postdrawfunc(s64ModelHelper* mdl, void (*postdraw)(u16))
{
    mdl->postdraw = postdraw;
}

void sausage64_update_anim(s64ModelHelper* mdl)
{
    const Animation* anim = mdl->curanim;
    const u32 animlen = anim->keyframes[anim->framecount-1].framenumber;
    const u32 curframeindex = mdl->curframeindex;
    const u32 nextframeindex = (curframeindex+1)%anim->framecount;
    
    // Update the frame index
    if (mdl->animtick >= anim->keyframes[nextframeindex].framenumber)
    {
        // Go to the next keyframe
        if (mdl->loop || mdl->animtick < animlen)
        {
            mdl->curframeindex = nextframeindex;
            mdl->animtick %= animlen;
        }
        else
            mdl->animtick = animlen-1;
    }
}

void sausage64_advance_anim(s64ModelHelper* mdl)
{
    if (mdl->nexttick < osGetTime())
    {
        mdl->animtick++;
        mdl->nexttick = osGetTime()+OS_NSEC_TO_CYCLES(33333333);
        sausage64_update_anim(mdl);
    }
}

void sausage64_set_anim(s64ModelHelper* mdl, u16 anim)
{
    Animation* animdata = &mdl->mdldata->anims[anim];
    mdl->curanim = animdata;
    mdl->curframeindex = 0;
    mdl->animtick = 0;
    mdl->nexttick = osGetTime()+OS_NSEC_TO_CYCLES(33333333);
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
    return a + f * (b - a);
}


/*==============================
    sausage64_drawpart
    Renders a part of a Sausage64 model
    @param A pointer to a display list pointer
    @param The model helper data
    @param The part of the model to render
==============================*/

static inline void sausage64_drawpart(Gfx** glistp, const u8 interpolate, const FrameData* cfdata, const FrameData* nfdata, const f32 l, Mtx* matrix, Gfx* dl)
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
    
    // Apply the matricies
    gSPMatrix((*glistp)++, OS_K0_TO_PHYSICAL(matrix), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);

    // Draw the body part
    gSPDisplayList((*glistp)++, dl);
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
    const u16 mcount = mdl->mdldata->meshcount;
    const Animation* anim = mdl->curanim;
    const KeyFrame* ckframe = &anim->keyframes[mdl->curframeindex]; // pass as an arg
    const KeyFrame* nkframe = &anim->keyframes[(mdl->curframeindex+1)%anim->framecount]; // pass as an arg
    f32 l = 0;
    
    // Prevent division by zero
    if (nkframe->framenumber - ckframe->framenumber != 0)
        l = ((f32)(mdl->animtick - ckframe->framenumber))/((f32)(nkframe->framenumber - ckframe->framenumber));
    
    for (i=0; i<mcount; i++)
    {
        const FrameData* cfdata = &ckframe->framedata[i];
        const FrameData* nfdata = &nkframe->framedata[i];
        
        // Call the pre draw function
        if (mdl->predraw != NULL)
            mdl->predraw(i);
            
        // Draw this part of the model
        sausage64_drawpart(glistp, mdl->interpolate, cfdata, nfdata, l, &mdl->matrix[i],  mdl->mdldata->meshes[i].dl);
        
        // Call the post draw function
        if (mdl->postdraw != NULL)
            mdl->postdraw(i);
    }
}