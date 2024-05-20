/***************************************************************
                          sausage64.c
                               
A very simple library that handles Sausage64 models exported by 
Arabiki64.
https://github.com/buu342/Blender-Sausage64
***************************************************************/

#include "sausage64.h"
#ifdef LIBDRAGON
    #include <math.h>
#endif
#include <stdlib.h>
#include <malloc.h>


/*********************************
  Libultra types (for libdragon)
*********************************/

#ifdef LIBDRAGON
    #ifndef TRUE
        #define TRUE 1
    #endif
    #ifndef FALSE
        #define FALSE 0
    #endif
#endif


/*********************************
             Structs
*********************************/

// Quaternion helper struct
typedef struct {
    f32 w;
    f32 x;
    f32 y;
    f32 z;
} s64Quat;


/*********************************
             Globals
*********************************/

#ifndef LIBDRAGON
    static f32 s64_viewmat[4][4];
    static f32 s64_projmat[4][4];
#else
    static Mtx s64_viewmat;
    static Mtx s64_projmat;
    static s64Material* s64_lastmat = NULL;
#endif


/*********************************
      Helper Math Functions
*********************************/

/*==============================
    s64clamp
    Clamps a value between two others
    Code from https://stackoverflow.com/questions/427477/fastest-way-to-clamp-a-real-fixed-floating-point-value/16659263#16659263
    @param The value to clamp
    @param The minimum value
    @param The maximum value
    @return The clamped value
==============================*/

static inline f32 s64clamp(f32 value, f32 min, f32 max)
{
    const f32 result = value < min ? min : value;
    return result > max ? max : result;
}


/*==============================
    s64lerp
    Returns the linear interpolation of 
    two values given a fraction
    @param The first value
    @param The target value
    @param The fraction
    @return The interpolated result
==============================*/

static inline f32 s64lerp(f32 a, f32 b, f32 f)
{
    return a + f*(b - a);
}


/*==============================
    s64quat_dot
    Calculates the dot product of
    two quaternions
    @param The first quaternion
    @param The target quaternion
    @return The dot product
==============================*/

static inline f32 s64quat_dot(s64Quat q1, s64Quat q2)
{
    return q1.x*q2.x + q1.y*q2.y + q1.z*q2.z + q1.w*q2.w;
}


/*==============================
    s64quat_normalize
    Normalizes a quaternion
    @param The quaternion to normalize
    @return The normalized quaternion
==============================*/

static inline f32 s64quat_normalize(s64Quat q)
{
    return sqrtf(s64quat_dot(q, q));
}


/*==============================
    s64quat_mul
    Multiply two quaternions together using Hamilton product.
    This combines their rotations.
    Order matters!
    @param The first quaternion
    @param The second quaternion
    @return The combined quaternion
==============================*/

static s64Quat s64quat_mul(s64Quat a, s64Quat b)
{
    s64Quat res = {
        a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z,
        a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
        a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
        a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w
    };
    return res;
}


/*==============================
    s64quat_difference
    Get the angle difference between two quaternions
    @param The first quaternion
    @param The second quaternion
    @return The quaternion representing the difference in angle
==============================*/

static inline s64Quat s64quat_difference(s64Quat a, s64Quat b)
{
    s64Quat a_inv = {a.w, -a.x, -a.y, -a.z};
    return s64quat_mul(b, a_inv);
}


/*==============================
    s64slerp
    Returns the spherical linear 
    interpolation of two quaternions
    given a fraction.
    @param The first quaternion
    @param The target quaternion
    @param The fraction
    @return The interpolated quaternion
==============================*/

static inline s64Quat s64slerp(s64Quat a, s64Quat b, f32 f)
{
    s64Quat result;
    const float dot = s64quat_dot(a, b);
    float scale = (dot >= 0) ? 1.0f : -1.0f;
    
    // Scale the quaternion
    result.w = b.w*scale;
    result.x = b.x*scale;
    result.y = b.y*scale;
    result.z = b.z*scale;
    
    // Perform linear interpolation
    result.w = s64lerp(a.w, result.w, f);
    result.x = s64lerp(a.x, result.x, f);
    result.y = s64lerp(a.y, result.y, f);
    result.z = s64lerp(a.z, result.z, f);

    // Normalize the quaternion
    scale = 1/s64quat_normalize(result);
    result.x *= scale;
    result.y *= scale;
    result.z *= scale;
    result.w *= scale;
    return result;
}


/*==============================
    s64quat_to_mtx
    Converts a quaternion to a rotation matrix
    @param The quaternion to convert
    @param The rotation matrix to modify
==============================*/

static inline void s64quat_to_mtx(s64Quat q, f32 dest[][4])
{
    float xx, yy, zz, xy, yz, xz, wx, wy, wz, norm, s = 0;
    
    // Normalize the quaternion, and then check for division by zero
    norm = s64quat_normalize(q);
    if (norm > 0)
        s = 2/norm;

    // Calculate helper values to reduce computations later
    xx = q.x*q.x*s;
    xy = q.x*q.y*s;
    xz = q.x*q.z*s;
    yy = q.y*q.y*s;
    yz = q.y*q.z*s;
    zz = q.z*q.z*s;
    wx = q.w*q.x*s;
    wy = q.w*q.y*s;
    wz = q.w*q.z*s;

    // Calculate the indices of the matrix
    dest[0][1] = xy + wz;
    dest[1][2] = yz + wx;
    dest[2][0] = xz + wy;
    dest[1][0] = xy - wz;
    dest[2][1] = yz - wx;
    dest[0][2] = xz - wy;

    // Calculate the diagonal of the matrix
    dest[0][0] = 1 - yy - zz;
    dest[1][1] = 1 - xx - zz;
    dest[2][2] = 1 - xx - yy;
    dest[3][3] = 1;

    // The rest of the matrix should be 0
    dest[3][0] = 0;
    dest[0][3] = 0;
    dest[1][3] = 0;
    dest[2][3] = 0;
    dest[3][1] = 0;
    dest[3][2] = 0;
}


/*==============================
    s64quat_fromdir
    Calculates a quaternion from a normalized direction vector (using the up vector)
    @param  The normalized direction vector
    @return The direction quaternion
==============================*/

static s64Quat s64quat_fromdir(float dir[3])
{
    float l;
    const float forward[3] = S64_FORWARDVEC;
    float w[3];
    s64Quat q;
    w[0] = (forward[1]*dir[2]) - (forward[2]*dir[1]);
    w[1] = (forward[2]*dir[0]) - (forward[0]*dir[2]);
    w[2] = (forward[0]*dir[1]) - (forward[1]*dir[0]);
    q.w = 1.0f + forward[0]*dir[0] + forward[1]*dir[1] + forward[2]*dir[2];
    q.x = w[0];
    q.y = w[1];
    q.z = w[2];
    l = 1/sqrtf(q.w*q.w + q.x*q.x + q.y*q.y +q.z*q.z);
    q.w *= l;
    q.x *= l;
    q.y *= l;
    q.z *= l;
    return q;
}


/*==============================
    s64quat_fromeuler
    Currently unused
    Converts a Euler angle (in radians)
    to a quaternion
    @param The euler yaw (in radians)
    @param The euler pitch (in radians)
    @param The euler roll (in radians)
    @return The calculated quaternion
==============================*/

static inline s64Quat s64quat_fromeuler(f32 yaw, f32 pitch, f32 roll)
{
    s64Quat q;
    const float c1 = cosf(yaw/2);
    const float s1 = sinf(yaw/2);
    const float c2 = cosf(pitch/2);
    const float s2 = sinf(pitch/2);
    const float c3 = cosf(roll/2);
    const float s3 = sinf(roll/2);
    const float c1c2 = c1*c2;
    const float s1s2 = s1*s2;
    q.w = c1c2*c3 - s1s2*s3;
    q.x = c1c2*s3 + s1s2*c3;
    q.y = s1*c2*c3 + c1*s2*s3;
    q.z = c1*s2*c3 - s1*c2*s3;
    return q;
}


/*==============================
    s64calc_billboard
    Calculate a billboard matrix
    @param The matrix to fill
==============================*/

static inline void s64calc_billboard(f32 mtx[4][4])
{
    mtx[0][0] = s64_viewmat[0][0];
    mtx[1][0] = s64_viewmat[0][1];
    mtx[2][0] = s64_viewmat[0][2];
    mtx[3][0] = 0;

    mtx[0][1] = s64_viewmat[1][0];
    mtx[1][1] = s64_viewmat[1][1];
    mtx[2][1] = s64_viewmat[1][2];
    mtx[3][1] = 0;

    mtx[0][2] = s64_viewmat[2][0];
    mtx[1][2] = s64_viewmat[2][1];
    mtx[2][2] = s64_viewmat[2][2];
    mtx[3][2] = 0;

    mtx[0][3] = 0;
    mtx[1][3] = 0;
    mtx[2][3] = 0;
    mtx[3][3] = 1;
}


/*==============================
    s64vec_rotate
    Rotate a vector using a quaternion
    @param The vector to rotate
    @param The rotation quaternion
    @param The vector to store the result in
==============================*/

static inline void s64vec_rotate(f32 vec[3], s64Quat rot, f32 result[3])
{
    s64Quat vecquat = {0, vec[0], vec[1], vec[2]};
    s64Quat rotinv = {rot.w, -rot.x, -rot.y, -rot.z};
    s64Quat res = s64quat_mul(s64quat_mul(rot, vecquat), rotinv);
    result[0] = res.x;
    result[1] = res.y;
    result[2] = res.z;
}


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

s64ModelHelper* sausage64_inithelper(s64ModelData* mdldata)
{
    // Start by allocating a model helper struct
    s64ModelHelper* mdl = (s64ModelHelper*)malloc(sizeof(s64ModelHelper));
    if (mdl == NULL)
        return NULL;

    // Initialize the newly allocated structure
    mdl->interpolate = TRUE;
    mdl->loop = TRUE;
    mdl->curkeyframe = 0;
    mdl->animtick = 0;
    mdl->rendercount = 1;
    mdl->predraw = NULL;
    mdl->postdraw = NULL;
    mdl->animcallback = NULL;
    mdl->mdldata = mdldata;

    // Set the the first animation if it exists, otherwise set the animation to NULL
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

    // Allocate space for the transform helper
    mdl->transforms = (s64FrameTransform*)calloc(sizeof(s64FrameTransform)*mdldata->meshcount, 1);
    if (mdl->transforms == NULL)
    {
        free(mdl);
        return NULL;
    }

    // Allocate space for the model matrices (or GLBuffers in Libdragon's case)
    #ifndef LIBDRAGON
        mdl->matrix = (Mtx*)malloc(sizeof(Mtx)*1); // TODO: Handle frame buffering properly. Will require a better API
        if (mdl->matrix == NULL)
        {
            free(mdl->transforms);
            free(mdl);
            return NULL;
        }
    #else
        // If the GL buffers haven't been initialized
        if (mdldata->glbuffers[0] == 0xFFFFFFFF)
        {
            int meshcount = mdldata->meshcount;

            // Generate the buffers, and then loop through and assign them to the vert+face arrays
            glGenBuffersARB(meshcount*2, mdldata->glbuffers);
            for (int i=0; i<meshcount; i++)
            {
                s64Gfx* dl = mdldata->meshes[i].dl;
                int facecount = 0, vertcount = 0;

                // Count the number of faces
                for (u32 j=0; j<dl->blockcount; j++)
                {
                    vertcount += dl->renders[j].vertcount;
                    facecount += dl->renders[j].facecount;
                }

                // Generate the buffers
                glBindBufferARB(GL_ARRAY_BUFFER_ARB, mdldata->glbuffers[i*2]);
                glBufferDataARB(GL_ARRAY_BUFFER_ARB, vertcount*sizeof(f32)*11, dl->renders[0].verts, GL_STATIC_DRAW_ARB);
                glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mdldata->glbuffers[i*2 + 1]);
                glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, facecount*sizeof(u16)*3, dl->renders[0].faces, GL_STATIC_DRAW_ARB);
            }
        }
    #endif
    return mdl;
}


/*==============================
    sausage64_set_camera
    Sets the camera for Sausage64 to use for billboarding
    @param The view matrix
    @param The projection matrix
==============================*/

void sausage64_set_camera(Mtx* view, Mtx* projection)
{
    #ifndef LIBDRAGON
        guMtxL2F(s64_viewmat, view);
        guMtxL2F(s64_projmat, projection);
    #else
        int i, j;
        for (i=0; i<4; i++)
        {
            for (j=0; j<4; j++)
            {
                s64_viewmat[i][j] = *view[i][j];
                s64_projmat[i][j] = *projection[i][j];
            }
        }
    #endif
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
    u32 curkf_index = mdl->curkeyframe;
    u32 nextkf_index = (curkf_index+1)%(anim->keyframecount);
    const u32 curkf_value = anim->keyframes[curkf_index].framenumber;
    
    // Check if we changed animation frame
    if (curtick < curkf_value || curtick >= anim->keyframes[nextkf_index].framenumber)
    {
        int advance = 1;
        if (curtick < curkf_value)
            advance = -1;
            
        // Cycle through all remaining, starting at the one after the current frame
        do
        {
            curkf_index += advance;
            nextkf_index += advance;
            if (curtick >= anim->keyframes[curkf_index].framenumber && curtick < anim->keyframes[nextkf_index].framenumber)
            {
                mdl->curkeyframe = curkf_index;
                return;
            }
        }
        while (1);
    }
}


/*==============================
    sausage64_advance_anim
    Advances the animation tick by the given amount
    @param The model helper pointer
    @param The amount to increase the animation tick by
==============================*/

void sausage64_advance_anim(s64ModelHelper* mdl, f32 tickamount)
{
    int rollover = 0;
    mdl->animtick += tickamount;
    
    // Check for rollover
    if (mdl->animtick >= mdl->curanimlen)
        rollover = 1;
    else if (mdl->animtick <= 0)
        rollover = -1;
    
    // If the animation ended, call the callback function and roll the tick value over
    if (rollover)
    {
        float division;
        const int animlength = mdl->curanimlen;
    
        // Execute the animation end callback function
        if (mdl->animcallback != NULL)
            mdl->animcallback(mdl->curanim - &mdl->mdldata->anims[0]);
        
        // If looping is disabled, then stop
        if (!mdl->loop)
        {
            if (rollover > 0)
            {
                mdl->animtick = (float)animlength;
                mdl->curkeyframe = mdl->curanim->keyframecount-1;
            }
            else
            {
                mdl->animtick = 0;
                mdl->curkeyframe = 0;
            }
            return;
        }
        
        // Calculate the correct tick
        division = mdl->animtick/((float)mdl->curanimlen);
        if (rollover > 0)
            mdl->animtick = (division - ((int)division))*((float)mdl->curanimlen);
        else
            mdl->animtick = (1+(division - ((int)division)))*((float)mdl->curanimlen);
    }

    // Update the animation
    if (mdl->curanim->keyframecount > 0)
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


#ifdef LIBDRAGON
    /*==============================
        sausage64_loadmaterial
        Loads a material for libdragon rendering
        @param The material to load
    ==============================*/
    
    void sausage64_loadmaterial(s64Material* mat)
    {
        if (mat->type == TYPE_TEXTURE)
        {
            s64Texture* tex = (s64Texture*)mat->data;
            if (s64_lastmat == NULL || (s64_lastmat->type != TYPE_TEXTURE))
            {
                const GLfloat diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
                glEnable(GL_TEXTURE_2D);
                glEnable(GL_COLOR_MATERIAL);
                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
            }
            glBindTexture(GL_TEXTURE_2D, *tex->identifier);
        }
        else
        {
            s64PrimColor* col = (s64PrimColor*)mat->data;
            if (s64_lastmat == NULL || (s64_lastmat->type != TYPE_PRIMCOL))
            {
                glDisable(GL_TEXTURE_2D);
                glDisable(GL_COLOR_MATERIAL);
            }
            const GLfloat diffuse[] = {(float)col->r/255.0f, (float)col->g/255.0f, (float)col->b/255.0f, 1.0f};
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
        }

        // Set other draw settings
        if (s64_lastmat == NULL || (s64_lastmat->cullback != mat->cullback || s64_lastmat->cullfront != mat->cullfront))
        {
            glEnable(GL_CULL_FACE);
            if (mat->cullfront && mat->cullback)
                glCullFace(GL_FRONT_AND_BACK);
            else if (mat->cullfront)
                glCullFace(GL_FRONT);
            else if (mat->cullback)
                glCullFace(GL_BACK);
            else
                glDisable(GL_CULL_FACE);
        }
        if (s64_lastmat == NULL || s64_lastmat->lighting != mat->lighting)
        {
            if (mat->lighting)
                glEnable(GL_LIGHTING);
            else
                glDisable(GL_LIGHTING);
        }
        if (s64_lastmat == NULL || s64_lastmat->depthtest != mat->depthtest)
        {
            if (mat->depthtest)
                glEnable(GL_DEPTH_TEST);
            else
                glDisable(GL_DEPTH_TEST);
        }
        if (s64_lastmat == NULL || s64_lastmat->smooth != mat->smooth)
        {
            if (mat->smooth)
                glShadeModel(GL_SMOOTH);
            else
                glShadeModel(GL_FLAT);
        }
        s64_lastmat = mat;
    }
#endif


/*==============================
    sausage64_calctransforms
    Calculates the transform of a mesh based on the animation
    @param A pointer to the model helper to use
    @param The mesh to calculate the transforms of
    @param The lerp amount
==============================*/

static void sausage64_calctransforms(s64ModelHelper* mdl, const u16 mesh, f32 l)
{
    // Prevent these calculations from being performed again
    if (mdl->transforms[mesh].rendercount == mdl->rendercount)
        return;
    mdl->transforms[mesh].rendercount = mdl->rendercount;

    // Calculate animation transforms
    if (mdl->curanim != NULL)
    {    
        s64Transform* fdata = &mdl->transforms[mesh].data;
        const s64Transform* cfdata = &mdl->curanim->keyframes[mdl->curkeyframe].framedata[mesh];
        
        // Calculate animation lerp
        if (mdl->interpolate)
        {
            const s64Transform* nfdata = &mdl->curanim->keyframes[(mdl->curkeyframe+1)%mdl->curanim->keyframecount].framedata[mesh];
            
            fdata->pos[0] = s64lerp(cfdata->pos[0], nfdata->pos[0], l);
            fdata->pos[1] = s64lerp(cfdata->pos[1], nfdata->pos[1], l);
            fdata->pos[2] = s64lerp(cfdata->pos[2], nfdata->pos[2], l);
            if (!mdl->mdldata->meshes[mesh].is_billboard)
            {
                s64Quat q =  {cfdata->rot[0], cfdata->rot[1], cfdata->rot[2], cfdata->rot[3]};
                s64Quat qn = {nfdata->rot[0], nfdata->rot[1], nfdata->rot[2], nfdata->rot[3]};
                q = s64slerp(q, qn, l);
                fdata->rot[0] = q.w;
                fdata->rot[1] = q.x;
                fdata->rot[2] = q.y;
                fdata->rot[3] = q.z;
            }
            fdata->scale[0] = s64lerp(cfdata->scale[0], nfdata->scale[0], l);
            fdata->scale[1] = s64lerp(cfdata->scale[1], nfdata->scale[1], l);
            fdata->scale[2] = s64lerp(cfdata->scale[2], nfdata->scale[2], l);
        }
        else
        {
            fdata->pos[0] = cfdata->pos[0];
            fdata->pos[1] = cfdata->pos[1];
            fdata->pos[2] = cfdata->pos[2];
            if (!mdl->mdldata->meshes[mesh].is_billboard)
            {
                fdata->rot[0] = cfdata->rot[0];
                fdata->rot[1] = cfdata->rot[1];
                fdata->rot[2] = cfdata->rot[2];
                fdata->rot[3] = cfdata->rot[3];
            }
            fdata->scale[0] = cfdata->scale[0];
            fdata->scale[1] = cfdata->scale[1];
            fdata->scale[2] = cfdata->scale[2];
        }
    }
}


/*==============================
    sausage64_calcanimlerp
    Calculates the lerp value based on the current animation
    @param  A pointer to the model helper to use
    @return The lerp amount
==============================*/

static f32 sausage64_calcanimlerp(s64ModelHelper* mdl)
{
    const s64Animation* anim = mdl->curanim;
    const s64KeyFrame* ckframe = &anim->keyframes[mdl->curkeyframe];
    const s64KeyFrame* nkframe = &anim->keyframes[(mdl->curkeyframe+1)%anim->keyframecount];

    // Prevent division by zero when calculating the lerp amount
    if (nkframe->framenumber - ckframe->framenumber != 0)
        return ((f32)(mdl->animtick - ckframe->framenumber))/((f32)(nkframe->framenumber - ckframe->framenumber));
    return 0;
}


/*==============================
    sausage64_get_meshtransform
    Get the current transform of the mesh,
    local to the object
    @param  The model helper pointer
    @param  The mesh to check
    @return The mesh's local transform
==============================*/

s64Transform* sausage64_get_meshtransform(s64ModelHelper* mdl, const u16 mesh)
{
    f32 l = sausage64_calcanimlerp(mdl);
    sausage64_calctransforms(mdl, mesh, l);
    return &mdl->transforms[mesh].data;
}


/*==============================
    sausage64_lookat
    Make a mesh look at another
    @param The model helper pointer
    @param The mesh to force the lookat
    @param The normalized direction vector of what we want to look at
    @param A value from 0.0 to 1.0 stating how much to look at the object
    @param Whether the lookat should propagate to the children meshes (up to one level)
==============================*/

void sausage64_lookat(s64ModelHelper* mdl, const u16 mesh, f32 dir[3], f32 amount, u8 affectchildren)
{
    s64Quat q, qt;
    s64Transform oldtrans_parent;
    s64Transform* trans;
    f32 l = sausage64_calcanimlerp(mdl);
    
    // First, ensure that the transforms for this mesh has been calculated
    sausage64_calctransforms(mdl, mesh, l);
    
    // Get the transform data
    trans = &mdl->transforms[mesh].data;
    if (affectchildren)
        oldtrans_parent = *trans;
    q.w = trans->rot[0];
    q.x = trans->rot[1];
    q.y = trans->rot[2];
    q.z = trans->rot[3];
    
    // Now, we can calculate the target quaternion, and lerp it
    qt = s64quat_fromdir(dir);
    qt = s64slerp(q, qt, amount);
    trans->rot[0] = qt.w;
    trans->rot[1] = qt.x;
    trans->rot[2] = qt.y;
    trans->rot[3] = qt.z;
    
    // Calculate the children's new transforms
    if (affectchildren)
    {
        int i;
        s64ModelData* mdata = mdl->mdldata;
        s64Quat rotdiff = s64quat_difference(q, qt);
        for (i=0; i<mdata->meshcount; i++)  
        {
            float root_offset[3];
            float root_offset_new[3];
            s64Transform* trans_child;
            s64Quat rot;
            if (mdata->meshes[i].parent != mesh)
                continue;
            sausage64_calctransforms(mdl, i, l);
            trans_child = &mdl->transforms[i].data;
            
            // Get the offset of the child's root compared to the parent's
            root_offset[0] = trans_child->pos[0] - oldtrans_parent.pos[0];
            root_offset[1] = trans_child->pos[1] - oldtrans_parent.pos[1];
            root_offset[2] = trans_child->pos[2] - oldtrans_parent.pos[2];
            
            // Rotate the root around the new rotational difference
            s64vec_rotate(root_offset, rotdiff, root_offset_new);
            
            // Apply the new root
            trans_child->pos[0] += root_offset_new[0] - root_offset[0];
            trans_child->pos[1] += root_offset_new[1] - root_offset[1];
            trans_child->pos[2] += root_offset_new[2] - root_offset[2];
            
            // Now add the rotation to the mesh itself
            rot.w = trans_child->rot[0];
            rot.x = trans_child->rot[1];
            rot.y = trans_child->rot[2];
            rot.z = trans_child->rot[3];
            rot = s64quat_mul(rotdiff, rot);
            trans_child->rot[0] = rot.w;
            trans_child->rot[1] = rot.x;
            trans_child->rot[2] = rot.y;
            trans_child->rot[3] = rot.z;
        }
    }
}


/*==============================
    sausage64_drawpart
    Renders a part of a Sausage64 model
    @param A pointer to a display list pointer
    @param The model helper to use
    @param The mesh to render
==============================*/

#ifndef LIBDRAGON
    static inline void sausage64_drawpart(Gfx** glistp, s64ModelHelper* mdl, u16 mesh)
    {
        float helper1[4][4];
        float helper2[4][4];
        s64Transform* fdata = &mdl->transforms[mesh].data;
        
        // Combine the translation and scale matrix
        guTranslateF(helper1, fdata->pos[0], fdata->pos[1], fdata->pos[2]);
        guScaleF(helper2, fdata->scale[0], fdata->scale[1], fdata->scale[2]);
        guMtxCatF(helper2, helper1, helper1);
        
        // Combine the rotation matrix
        if (!mdl->mdldata->meshes[mesh].is_billboard)
        {
            s64Quat q = {fdata->rot[0], fdata->rot[1], fdata->rot[2], fdata->rot[3]};
            s64quat_to_mtx(q, helper2);
        }
        else
            s64calc_billboard(helper2);
        guMtxCatF(helper2, helper1, helper1);
        
        // Draw the body part
        guMtxF2L(helper1, &mdl->matrix[mesh]);
        gSPMatrix((*glistp)++, OS_K0_TO_PHYSICAL(&mdl->matrix[mesh]), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
        gSPDisplayList((*glistp)++, mdl->mdldata->meshes[mesh].dl);
        gSPPopMatrix((*glistp)++, G_MTX_MODELVIEW);
    }
#else
    static inline void sausage64_drawpart(s64Gfx* dl, s64ModelHelper* mdl, u16 mesh)
    {
        float helper1[4][4];
        s64Transform* fdata = &mdl->transforms[mesh].data;
    
        // Push a new matrix
        glPushMatrix();
        
        // Combine the translation and scale matrix
        glTranslatef(fdata->pos[0], fdata->pos[1], fdata->pos[2]);
        glScalef(fdata->scale[0], fdata->scale[1], fdata->scale[2]);

        // Combine the rotation matrix
        if (!mdl->mdldata->meshes[mesh].is_billboard)
        {
            s64Quat q = {fdata->rot[0], fdata->rot[1], fdata->rot[2], fdata->rot[3]};
            s64quat_to_mtx(q, helper1);
        }
        else
            s64calc_billboard(helper1);
        glMultMatrixf(&helper1[0][0]);

        // Draw the body part
        for (u32 j=0; j<dl->blockcount; j++)
        {
            s64RenderBlock* render = &dl->renders[j];
            int fc = render->facecount;
            if (render->material != NULL && render->material != s64_lastmat)
                sausage64_loadmaterial(render->material);
            glVertexPointer(3, GL_FLOAT, sizeof(f32)*11, (u8*)(0*sizeof(f32)));
            glTexCoordPointer(2, GL_FLOAT, sizeof(f32)*11, (u8*)(3*sizeof(f32)));
            glNormalPointer(GL_FLOAT, sizeof(f32)*11, (u8*)(5*sizeof(f32)));
            glColorPointer(3, GL_FLOAT, sizeof(f32)*11, (u8*)(8*sizeof(f32)));
            glDrawElements(GL_TRIANGLES, fc * 3, GL_UNSIGNED_SHORT, (u8*)(3*sizeof(u16)*(render->faces - dl->renders[0].faces)));
        }
        glPopMatrix();
    }
#endif


/*==============================
    sausage64_drawmodel
    Renders a Sausage64 model
    @param (Libultra) A pointer to a display list pointer
            (Libdragon) The model helper data
    @param (Libultra) The model helper data
==============================*/

#ifndef LIBDRAGON
    void sausage64_drawmodel(Gfx** glistp, s64ModelHelper* mdl)
    {
        u16 i;
        f32 l = 0;
        const s64ModelData* mdata = mdl->mdldata;
        const u16 mcount = mdata->meshcount;
        const s64Animation* anim = mdl->curanim;
    
        // If we have a valid animation, get the lerp value
        if (anim != NULL)
            l = sausage64_calcanimlerp(mdl);
        
        // Iterate through each mesh
        for (i=0; i<mcount; i++)
        {           
            // Call the pre draw function
            if (mdl->predraw != NULL)
                mdl->predraw(i);
            
            // Draw this part of the model
            if (anim != NULL)
            {
                sausage64_calctransforms(mdl, i, l);
                sausage64_drawpart(glistp, mdl, i);
            }
            else
                gSPDisplayList((*glistp)++, mdata->meshes[i].dl);
        
            // Call the post draw function
            if (mdl->postdraw != NULL)
                mdl->postdraw(i);
        }

        // Increment the render count for transform calculations
        mdl->rendercount++;
    }
#else
    void sausage64_drawmodel(s64ModelHelper* mdl)
    {
        u16 i;
        f32 l = 0;
        const s64ModelData* mdata = mdl->mdldata;
        const u16 mcount = mdata->meshcount;
        const s64Animation* anim = mdl->curanim;

        // Initialize OpenGL state
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
    
        // If we have a valid animation, get the lerp value
        if (anim != NULL)
            l = sausage64_calcanimlerp(mdl);
        
        // Iterate through each mesh
        for (i=0; i<mcount; i++)
        {           
            // Call the pre draw function
            if (mdl->predraw != NULL)
                mdl->predraw(i);
            
            // Draw this part of the model
            if (anim != NULL)
            {
                s64Gfx* dl = mdl->mdldata->meshes[i].dl;
                sausage64_calctransforms(mdl, i, l);
                glBindBufferARB(GL_ARRAY_BUFFER_ARB, mdata->glbuffers[i*2]);
                glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mdata->glbuffers[i*2 + 1]);
                sausage64_drawpart(dl, mdl, i);
            }
            //else
            //    gSPDisplayList((*glistp)++, mdata->meshes[i].dl);
        
            // Call the post draw function
            if (mdl->postdraw != NULL)
                mdl->postdraw(i);
        }

        // Increment the render count for transform calculations
        mdl->rendercount++;
        /*
        u16 i;
        const s64ModelData* mdata = mdl->mdldata;
        const u16 mcount = mdata->meshcount;
        const s64Animation* anim = mdl->curanim;

        // Initialize OpenGL state
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);

        // Increment the render count for transform calculations
        mdl->rendercount++;

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
                const s64Transform* cfdata = &ckframe->framedata[i];
                const s64Transform* nfdata = &nkframe->framedata[i];

                // Call the pre draw function
                if (mdl->predraw != NULL)
                    mdl->predraw(i);

                // Draw this part of the model without animations
                s64Gfx* dl = mdl->mdldata->meshes[i].dl;
                glBindBufferARB(GL_ARRAY_BUFFER_ARB, mdata->glbuffers[i*2]);
                glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mdata->glbuffers[i*2 + 1]);
                sausage64_drawpart(dl, &mdata->meshes[i], cfdata, nfdata, mdl->interpolate, l);
            }

            // Call the post draw function
            if (mdl->postdraw != NULL)
                mdl->postdraw(i);
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
                s64Gfx* dl = mdl->mdldata->meshes[i].dl;
                glBindBufferARB(GL_ARRAY_BUFFER_ARB, mdata->glbuffers[i*2]);
                glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mdata->glbuffers[i*2 + 1]);
                for (u32 j=0; j<dl->blockcount; j++)
                {
                    s64RenderBlock* render = &dl->renders[j];
                    int fc = render->facecount;
                    if (render->material != NULL && render->material != s64_lastmat)
                        sausage64_loadmaterial(render->material);
                    glVertexPointer(3, GL_FLOAT, sizeof(f32)*11, (u8*)(0*sizeof(f32)));
                    glTexCoordPointer(2, GL_FLOAT, sizeof(f32)*11, (u8*)(3*sizeof(f32)));
                    glNormalPointer(GL_FLOAT, sizeof(f32)*11, (u8*)(5*sizeof(f32)));
                    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(f32)*11, (u8*)(8*sizeof(f32)));
                    glDrawElements(GL_TRIANGLES, fc*3, GL_UNSIGNED_SHORT, (u8*)(3*sizeof(u16)*(render->faces - dl->renders[0].faces)));
                }

                // Call the post draw function
                if (mdl->postdraw != NULL)
                    mdl->postdraw(i);
            }
        }
        */
    }
#endif

/*==============================
    sausage64_freehelper
    Frees the memory used up by a Sausage64 model helper
    @param A pointer to the model helper
==============================*/

void sausage64_freehelper(s64ModelHelper* helper)
{
    free(helper->transforms);
    #ifndef LIBDRAGON
        free(helper->matrix);
    #endif
    free(helper);
}