/***************************************************************
                          sausage64.c
                               
A very library that handles Sausage64 models exported by 
Arabiki64.
https://github.com/buu342/Blender-Sausage64
***************************************************************/

#include <ultra64.h>
#include "sausage64.h"

#define DTOR (3.1415926/180.0)

typedef struct {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
} s64Quat;

static void quaternion_fromeuler(s64Quat* q, float yaw, float pitch, float roll)
{
    const float c1 = cosf(yaw/2);
    const float s1 = sinf(yaw/2);
    const float c2 = cosf(pitch/2);
    const float s2 = sinf(pitch/2);
    const float c3 = cosf(roll/2);
    const float s3 = sinf(roll/2);
    const float c1c2 = c1*c2;
    const float s1s2 = s1*s2;
    q->w = c1c2*c3 - s1s2*s3;
    q->x = c1c2*s3 + s1s2*c3;
    q->y = s1*c2*c3 + c1*s2*s3;
    q->z = c1*s2*c3 - s1*c2*s3;
}

static inline float quaternion_dot(s64Quat q1, s64Quat q2) {
    return q1.x*q2.x + q1.y*q2.y + q1.z*q2.z + q1.w*q2.w;
}

static inline float quaternion_normalize(s64Quat q) {
    return sqrtf(quaternion_dot(q, q));
}

static void quaternion_to_mtx(s64Quat* q, float dest[][4])
{
    float w, x, y, z, xx, yy, zz, xy, yz, xz, wx, wy, wz, norm, s;

    guMtxIdentF(dest);
    norm = quaternion_normalize(*q);
    s    = norm > 0 ? 2 / norm : 0;

    x = q->x;
    y = q->y;
    z = q->z;
    w = q->w;

    xx = s * x * x;
    xy = s * x * y;
    xz = s * x * z;
    yy = s * y * y;
    yz = s * y * z;
    zz = s * z * z;
    wx = s * w * x;
    wy = s * w * y;
    wz = s * w * z;

    dest[0][0] = 1 - yy - zz;
    dest[1][1] = 1 - xx - zz;
    dest[2][2] = 1 - xx - yy;

    dest[0][1] = xy + wz;
    dest[1][2] = yz + wx;
    dest[2][0] = xz + wy;

    dest[1][0] = xy - wz;
    dest[2][1] = yz - wx;
    dest[0][2] = xz - wy;

    dest[0][3] = 0;
    dest[1][3] = 0;
    dest[2][3] = 0;
    dest[3][0] = 0;
    dest[3][1] = 0;
    dest[3][2] = 0;
    dest[3][3] = 1;
}


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

#include "debug.h"
static inline s64Quat s64slerp(s64Quat a, s64Quat b, f32 f)
{
    s64Quat result, q1, q2;
    float cosTheta, sinTheta, angle, s;
    
    if (a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w)
        return a;
    
    cosTheta = quaternion_dot(a, b);
    sinTheta = sqrtf(1 - cosTheta*cosTheta);
    
    // lerp to avoid division by zero
    if (sinTheta < 0.001 || sinTheta > -0.001)
    {
        result.x = s64lerp(a.x, b.x, f);
        result.y = s64lerp(a.y, b.y, f);
        result.z = s64lerp(a.z, b.z, f);
        result.w = s64lerp(a.w, b.w, f);
        return result;
    }

    // Spherical lerp
    angle = acos(cosTheta);
    s = sinf((1 - f)*angle);
    q1.x = a.x*s;
    q1.y = a.y*s;
    q1.z = a.z*s;
    q1.w = a.w*s;
    
    s = sinf(f*angle);
    q2.x = b.x*s;
    q2.y = b.y*s;
    q2.z = b.z*s;
    q2.w = b.w*s;
    
    q1.x += q1.x + q2.x;
    q1.y += q1.y + q2.y;
    q1.z += q1.z + q2.z;
    q1.w += q1.w + q2.w;

    s = 1/sinTheta;
    result.x = q1.x*s;
    result.y = q1.y*s;
    result.z = q1.z*s;
    result.w = q1.w*s;
    return result;
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
    float fhelper[4][4];

    // Initialize the matricies
    guMtxIdent(matrix);
    guMtxIdent(&helper);
    
    // Calculate the transformations on the CPU
    if (interpolate)
    {
        s64Quat q1, q2;
        const float r1 = cfdata->rot[0]*DTOR;
        const float p1 = cfdata->rot[1]*DTOR;
        const float h1 = cfdata->rot[2]*DTOR;
        const float r2 = nfdata->rot[0]*DTOR;
        const float p2 = nfdata->rot[1]*DTOR;
        const float h2 = nfdata->rot[2]*DTOR;
        quaternion_fromeuler(&q1, p1, h1, r1);
        quaternion_fromeuler(&q2, p2, h2, r2);
        q1 = s64slerp(q1, q2, l);
        
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
        quaternion_to_mtx(&q1, fhelper);
        guMtxF2L(fhelper, &helper);
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