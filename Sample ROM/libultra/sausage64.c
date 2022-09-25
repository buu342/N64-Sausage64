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
    static float s64_viewmat[4][4];
    static float s64_projmat[4][4];
#else
    static Mtx* s64_viewmat = NULL;
    static Mtx* s64_projmat = NULL;
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
    @param The fraction
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

static inline float s64quat_dot(s64Quat q1, s64Quat q2)
{
    return q1.x*q2.x + q1.y*q2.y + q1.z*q2.z + q1.w*q2.w;
}


/*==============================
    s64quat_normalize
    Normalizes a quaternion
    @param The quaternion to normalize
==============================*/

static inline float s64quat_normalize(s64Quat q)
{
    return sqrtf(s64quat_dot(q, q));
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

static inline void s64quat_to_mtx(s64Quat q, float dest[][4])
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
    s64quat_fromeuler
    Currently unused
    Converts a Euler angle (in radians)
    to a quaternion
    @param The euler yaw (in radians)
    @param The euler pitch (in radians)
    @param The euler roll (in radians)
==============================*/

static inline s64Quat s64quat_fromeuler(float yaw, float pitch, float roll)
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


/*********************************
       Sausage64 Functions
*********************************/

/*==============================
    sausage64_initmodel
    Initialize a model helper struct
    @param The model helper to initialize
    @param The model data 
    @param (Libultra) An array of matrices for each mesh
           part
           (Libdragon) An array of GL buffers for each 
           mesh's verticies and faces
    ==============================*/

#ifndef LIBDRAGON
void sausage64_initmodel(s64ModelHelper* mdl, s64ModelData* mdldata, Mtx* matrices)
#else
void sausage64_initmodel(s64ModelHelper* mdl, s64ModelData* mdldata, GLuint* glbuffers)
#endif
{
    mdl->interpolate = TRUE;
    mdl->loop = TRUE;
    mdl->curkeyframe = 0;

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

    // Initialize the rest of the data
    mdl->animtick = 0;
    mdl->predraw = NULL;
    mdl->postdraw = NULL;
    mdl->animcallback = NULL;
    mdl->mdldata = mdldata;
    #ifndef LIBDRAGON
        mdl->matrix = matrices;
    #else
        mdl->glbuffers = glbuffers;

        // If the GL buffers haven't been initialized
        if (glbuffers[0] == 0xFFFFFFFF)
        {
            int meshcount = mdldata->meshcount;

            // Generate the buffers, and then loop through and assign them to the vert+face arrays
            glGenBuffersARB(meshcount*2, glbuffers);
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
                glBindBufferARB(GL_ARRAY_BUFFER_ARB, glbuffers[i*2]);
                glBufferDataARB(GL_ARRAY_BUFFER_ARB, vertcount*sizeof(f32)*11, dl->renders[0].verts, GL_STATIC_DRAW_ARB);
                glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, glbuffers[i*2 + 1]);
                glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, facecount*sizeof(u16)*3, dl->renders[0].faces, GL_STATIC_DRAW_ARB);
            }
        }
    #endif
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
        s64_viewmat = view;
        s64_projmat = projection;
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
    const u32 curkeyframe = mdl->curkeyframe;
    const u32 curframenum = anim->keyframes[curkeyframe].framenumber;
    const u32 nframes = anim->keyframecount;
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
            mdl->animtick = (float)mdl->curanimlen;
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
    sausage64_drawpart
    Renders a part of a Sausage64 model
    @param A pointer to a display list pointer
    @param The mesh to draw
    @param The current framedata
    @param The next framedata
    @param Whether to interpolate or not
    @param The interpolation amount
    @param (Libultra) The matrix to store the mesh's transformation
==============================*/

#ifndef LIBDRAGON
    static inline void sausage64_drawpart(Gfx** glistp, s64Mesh* mesh, const s64FrameData* cfdata, const s64FrameData* nfdata, const u8 interpolate, const f32 l, Mtx* matrix)
    {
        float helper1[4][4];
        float helper2[4][4];
        s64Quat q = {cfdata->rot[0], cfdata->rot[1], cfdata->rot[2], cfdata->rot[3]};
    
        // Calculate the transformations on the CPU
        if (interpolate)
        {
            // Setup the quaternions
            if (!mesh->is_billboard)
            {
                s64Quat qn = {nfdata->rot[0], nfdata->rot[1], nfdata->rot[2], nfdata->rot[3]};
                q = s64slerp(q, qn, l);
            }
        
            // Combine the translation and scale matrix
            guTranslateF(helper1, 
                s64lerp(cfdata->pos[0], nfdata->pos[0], l),
                s64lerp(cfdata->pos[1], nfdata->pos[1], l),
                s64lerp(cfdata->pos[2], nfdata->pos[2], l)
            );
            guScaleF(helper2, 
                s64lerp(cfdata->scale[0], nfdata->scale[0], l), 
                s64lerp(cfdata->scale[1], nfdata->scale[1], l), 
                s64lerp(cfdata->scale[2], nfdata->scale[2], l)
            );
            guMtxCatF(helper2, helper1, helper1);
        }
        else
        {
            // Combine the translation and scale matrix
            guTranslateF(helper1, 
                cfdata->pos[0],
                cfdata->pos[1],
                cfdata->pos[2]
            );
            guScaleF(helper2, cfdata->scale[0], cfdata->scale[1], cfdata->scale[2]);
            guMtxCatF(helper2, helper1, helper1);
        }
        
        // Combine the rotation matrix
        if (!mesh->is_billboard)
        {
            s64quat_to_mtx(q, helper2);
            guMtxCatF(helper2, helper1, helper1);
        }
        else
        {
            helper2[0][0] = s64_viewmat[0][0];
            helper2[1][0] = s64_viewmat[0][1];
            helper2[2][0] = s64_viewmat[0][2];
            helper2[3][0] = 0;
        
            helper2[0][1] = s64_viewmat[1][0];
            helper2[1][1] = s64_viewmat[1][1];
            helper2[2][1] = s64_viewmat[1][2];
            helper2[3][1] = 0;
        
            helper2[0][2] = s64_viewmat[2][0];
            helper2[1][2] = s64_viewmat[2][1];
            helper2[2][2] = s64_viewmat[2][2];
            helper2[3][2] = 0;
        
            helper2[0][3] = 0;
            helper2[1][3] = 0;
            helper2[2][3] = 0;
            helper2[3][3] = 1;
            guMtxCatF(helper2, helper1, helper1);
        }
        guMtxF2L(helper1, matrix);
    
        // Draw the body part
        gSPMatrix((*glistp)++, OS_K0_TO_PHYSICAL(matrix), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
        gSPDisplayList((*glistp)++, mesh->dl);
        gSPPopMatrix((*glistp)++, G_MTX_MODELVIEW);
    }
#else
    static inline void sausage64_drawpart(s64Gfx* dl, s64Mesh* mesh, const s64FrameData* cfdata, const s64FrameData* nfdata, const u8 interpolate, const f32 l)
    {
        float helper1[4][4];
        s64Quat q = {cfdata->rot[0], cfdata->rot[1], cfdata->rot[2], cfdata->rot[3]};
    
        // Push a new matrix
        glPushMatrix();

        // Calculate the transformations on the CPU
        if (interpolate)
        {
            // Setup the quaternions
            if (!mesh->is_billboard)
            {
                s64Quat qn = { nfdata->rot[0], nfdata->rot[1], nfdata->rot[2], nfdata->rot[3] };
                q = s64slerp(q, qn, l);
            }

            // Combine the translation and scale matrix
            glTranslatef(
                s64lerp(cfdata->pos[0], nfdata->pos[0], l),
                s64lerp(cfdata->pos[1], nfdata->pos[1], l),
                s64lerp(cfdata->pos[2], nfdata->pos[2], l)
            );
            glScalef(
                s64lerp(cfdata->scale[0], nfdata->scale[0], l),
                s64lerp(cfdata->scale[1], nfdata->scale[1], l),
                s64lerp(cfdata->scale[2], nfdata->scale[2], l)
            );
        }
        else
        {
            // Combine the translation and scale matrix
            glTranslatef(
                cfdata->pos[0],
                cfdata->pos[1],
                cfdata->pos[2]
            );
            glScalef(cfdata->scale[0], cfdata->scale[1], cfdata->scale[2]);
        }

        // Combine the rotation matrix
        if (!mesh->is_billboard)
        {
            s64quat_to_mtx(q, helper1);
        }
        else
        {
            helper1[0][0] = (*s64_viewmat)[0][0];
            helper1[1][0] = (*s64_viewmat)[0][1];
            helper1[2][0] = (*s64_viewmat)[0][2];
            helper1[3][0] = 0;
            
            helper1[0][1] = (*s64_viewmat)[1][0];
            helper1[1][1] = (*s64_viewmat)[1][1];
            helper1[2][1] = (*s64_viewmat)[1][2];
            helper1[3][1] = 0;
            
            helper1[0][2] = (*s64_viewmat)[2][0];
            helper1[1][2] = (*s64_viewmat)[2][1];
            helper1[2][2] = (*s64_viewmat)[2][2];
            helper1[3][2] = 0;
            
            helper1[0][3] = 0;
            helper1[1][3] = 0;
            helper1[2][3] = 0;
            helper1[3][3] = 1;
        }
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
                sausage64_drawpart(glistp, &mdata->meshes[i], cfdata, nfdata, mdl->interpolate, l, &mdl->matrix[i]);
            
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
#else
    void sausage64_drawmodel(s64ModelHelper* mdl)
    {
        u16 i;
        const s64ModelData* mdata = mdl->mdldata;
        const u16 mcount = mdata->meshcount;
        const s64Animation* anim = mdl->curanim;

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);

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

                // Draw this part of the model without animations
                s64Gfx* dl = mdl->mdldata->meshes[i].dl;
                glBindBufferARB(GL_ARRAY_BUFFER_ARB, mdl->glbuffers[i*2]);
                glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mdl->glbuffers[i*2 + 1]);
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
                glBindBufferARB(GL_ARRAY_BUFFER_ARB, mdl->glbuffers[i*2]);
                glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mdl->glbuffers[i*2 + 1]);
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
    }
#endif