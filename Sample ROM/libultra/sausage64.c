/***************************************************************
                          sausage64.c
                               
A very simple library that handles Sausage64 models exported by 
Arabiki64.
https://github.com/buu342/Blender-Sausage64
***************************************************************/

#include "sausage64.h"
#ifdef LIBDRAGON
    #include <math.h>
    #include <rdpq_tex.h>
    #include <asset.h>
#endif
#include <stdlib.h>
#include <malloc.h>
#include <string.h>


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
       Binary Asset Macros
*********************************/

#define BINARY_VERSION 0

// Custom Combine LERP function that doesn't do macro hackery
#ifndef LIBDRAGON
    #define	gDPSetCombineLERP_Custom(pkt, a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1) \
    { \
        Gfx *_g = (Gfx *)(pkt); \
        _g->words.w0 = _SHIFTL(G_SETCOMBINE, 24, 8) | _SHIFTL(GCCc0w0(a0, c0, Aa0, Ac0) | GCCc1w0(a1, c1), 0, 24); \
    	_g->words.w1 =	(unsigned int)(GCCc0w1(b0, d0, Ab0, Ad0) | GCCc1w1(b1, Aa1, Ac1, d1, Ab1, Ad1)); \
    }
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

// Binary model helper structs
typedef struct {
    char header[4];
    u16 count_meshes;
    u16 count_materials;
    u16 count_anims;
    u16 offset_meshes;
    u32 offset_materials;
    u32 offset_anims;
} BinFile_Header;

typedef struct {
    u32 meshdata_offset;
    u32 meshdata_size;
    u32 vertdata_offset;
    u32 vertdata_size;
    u32 facedata_offset;
    u32 facedata_size;
    u32 dldata_offset;
    u32 dldata_size;
    u32 dldata_slotcount;
} BinFile_TOC_Meshes;

typedef struct {
    s16   parent;
    u8    is_billboard;
    char* name;
} BinFile_MeshData;

typedef struct {
    u32 matdata_offset;
    u32 matdata_size;
    u32 material_offset;
    u32 material_size;
} BinFile_TOC_Materials;

typedef struct {
    u8 type;
    u8 lighting;
    u8 cullfront;
    u8 cullback;
    u8 smooth;
    u8 depthtest;
    char* name;
} BinFile_MatData;

typedef struct {
    u32 animdata_offset;
    u32 animdata_size;
    u32 kfdata_offset;
    u32 kfdata_size;
} BinFile_TOC_Anims;

typedef struct {
    u32 kfcount;
    u16* kfindices;
    char* name;
} BinFile_AnimData;


/*********************************
             Enum
*********************************/

// Libultra DL command enums
typedef enum {
    DPFillRectangle = 0,
    DPScisFillRectangle,
    DPFullSync,
    DPloSync,
    DPTileSync,
    DPPipeSync,
    DPLoadTLUT_pal16,
    DPLoadTLUT_pal256,
    DPLoadTextureBlock,
    DPLoadTextureBlock_4b,
    DPLoadTextureTile,
    DPLoadTextureTile_4b,
    DPLoadBlock,
    DPNoOp,
    DPNoOpTag,
    DPPipelineMode,
    DPSetBlendColor,
    DPSetEnvColor,
    DPSetFillColor,
    DPSetFogColor,
    DPSetPrimColor,
    DPSetColorImage,
    DPSetDepthImage,
    DPSetTextureImage,
    DPSetHilite1Tile,
    DPSetHilite2Tile,
    DPSetAlphaCompare,
    DPSetAlphaDither,
    DPSetColorDither,
    DPSetCombineMode,
    DPSetCombineLERP,
    DPSetConvert,
    DPSetTextureConvert,
    DPSetCycleType,
    DPSetDepthSource,
    DPSetCombineKey,
    DPSetKeyGB,
    DPSetKeyR,
    DPSetPrimDepth,
    DPSetRenderMode,
    DPSetScissor,
    DPSetTextureDetail,
    DPSetTextureFilter,
    DPSetTextureLOD,
    DPSetTextureLUT,
    DPSetTexturePersp,
    DPSetTile,
    DPSetTileSize,
    SP1Triangle,
    SP2Triangles,
    SPBranchLessZ,
    SPBranchLessZrg,
    SPBranchList,
    SPClipRatio,
    SPCullDisplayList,
    SPDisplayList,
    SPEndDisplayList,
    SPFogPosition,
    SPForceMatrix,
    SPSetGeometryMode,
    SPClearGeometryMode,
    SPInsertMatrix,
    SPLine3D,
    SPLineW3D,
    SPLoadUcode,
    SPLoadUcodeL,
    SPLookAt,
    SPMatrix,
    SPModifyVertex,
    SPPerspNormalize,
    SPPopMatrix,
    SPSegment,
    SPSetLights0,
    SPSetLights1,
    SPSetLights2,
    SPSetLights3,
    SPSetLights4,
    SPSetLights5,
    SPSetLights6,
    SPSetLights7,
    SPSetStatus,
    SPNumLights,
    SPLight,
    SPLightColor,
    SPTexture,
    SPTextureRectangle,
    SPScisTextureRectangle,
    SPTextureRectangleFlip,
    SPVertex,
    SPViewport,
    SPBgRectCopy,
    SPBgRect1Cyc,
    SPObjRectangle,
    SPObjRectangleR,
    SPObjSprite,
    SPObjMatrix,
    SPObjSubMatrix,
    SPObjRenderMode,
    SPObjLoadTxtr,
    SPObjLoadTxRect,
    SPObjLoadTxRectR,
    SPObjLoadTxSprite,
    SPSelectDL,
    SPSelectBranchDL
} DListCName;


/*********************************
             Globals
*********************************/

#ifndef LIBDRAGON
    static f32 s64_viewmat[4][4];
    static f32 s64_projmat[4][4];
#else
    static f32 s64_campos[3];
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
    const f32 dot = s64quat_dot(a, b);
    f32 scale = (dot >= 0) ? 1.0f : -1.0f;
    
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
    f32 xx, yy, zz, xy, yz, xz, wx, wy, wz, norm, s = 0;
    
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

static s64Quat s64quat_fromdir(f32 dir[3])
{
    f32 l;
    const f32 forward[3] = S64_FORWARDVEC;
    f32 w[3];
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
    const f32 c1 = cosf(yaw/2);
    const f32 s1 = sinf(yaw/2);
    const f32 c2 = cosf(pitch/2);
    const f32 s2 = sinf(pitch/2);
    const f32 c3 = cosf(roll/2);
    const f32 s3 = sinf(roll/2);
    const f32 c1c2 = c1*c2;
    const f32 s1s2 = s1*s2;
    q.w = c1c2*c3 - s1s2*s3;
    q.x = c1c2*s3 + s1s2*c3;
    q.y = s1*c2*c3 + c1*s2*s3;
    q.z = c1*s2*c3 - s1*c2*s3;
    return q;
}


#ifndef LIBDRAGON
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
#else
    /*==============================
        s64calc_billboard
        Calculate a billboard matrix
        @param The matrix to fill
        @param The model helper
        @param The mesh to billboard
    ==============================*/

    static inline void s64calc_billboard(f32 mtx[4][4], s64ModelHelper* mdl, u16 mesh)
    {
        f32 w;
        f32 dir[3];
        s64Quat q = {0.525322, 0.850904, 0.0, 0.0};
        s64Quat looked;
        s64Transform* trans = &mdl->transforms[mesh].data;

        // In libdragon, since we can't retrieve matrix states, we gotta calculate billboarding manually
        // Luckily, the lookat function will do the heavy lifting, we just need to get the direction of the camera
        dir[0] = s64_campos[0] - trans->pos[0];
        dir[1] = s64_campos[1] - trans->pos[1];
        dir[2] = s64_campos[2] - trans->pos[2];
        w = sqrtf(dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2]);
        if (w != 0)
            w = 1/w;
        else
            w = 0;
        dir[0] *= w;
        dir[1] *= w;
        dir[2] *= w;

        // Use lookat
        sausage64_lookat(mdl, mesh, dir, 1.0f, FALSE);

        // Rotate the mesh 90 degrees to match the billboarding in Libultra
        looked.w = trans->rot[0];
        looked.x = trans->rot[1];
        looked.y = trans->rot[2];
        looked.z = trans->rot[3];
        q = s64quat_mul(looked, q);
        trans->rot[0] = q.w;
        trans->rot[1] = q.x;
        trans->rot[2] = q.y;
        trans->rot[3] = q.z;
    }
#endif


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
     Asset Loading Functions
*********************************/

#ifdef LIBDRAGON
    /*==============================
        sausage64_load_texture
        Generates a texture for OpenGL.
        Since the s64Texture struct contains a bunch of information,
        this function lets us create these textures with the correct
        attributes automatically.
        @param The Sausage64 texture
        @param The texture data itself, in a sprite struct
    ==============================*/

    void sausage64_load_texture(s64Texture* tex, sprite_t* texture)
    {
        int repeats = 0, repeatt = 0, mirrors = MIRROR_NONE, mirrort = MIRROR_NONE;

        // Create the texture buffer 
        glGenTextures(1, tex->identifier);
        glBindTexture(GL_TEXTURE_2D, *tex->identifier);

        // Set the texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tex->wraps);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tex->wrapt);

        // Set the clamping values manually because SpriteTexture overrides TexParameter
        switch (tex->wraps)
        {
            case GL_MIRRORED_REPEAT_ARB: repeats = REPEAT_INFINITE; mirrors = MIRROR_REPEAT; break;
            case GL_REPEAT: repeats = REPEAT_INFINITE; mirrors = MIRROR_NONE; break;
        }
        switch (tex->wrapt)
        {
            case GL_MIRRORED_REPEAT_ARB: repeatt = REPEAT_INFINITE; mirrort = MIRROR_REPEAT; break;
            case GL_REPEAT: repeatt = REPEAT_INFINITE; mirrort = MIRROR_NONE; break;
        }

        // Make the texture from the sprite
        glSpriteTextureN64(GL_TEXTURE_2D, texture, &(rdpq_texparms_t){
            .s.repeats = repeats, 
            .s.mirror = mirrors, 
            .t.repeats = repeatt,
            .t.mirror = mirrort
        });
    }


    /*==============================
        sausage64_unload_texture
        Unloads a texture created for OpenGL
        @param The s64 texture to unload
    ==============================*/

    void sausage64_unload_texture(s64Texture* tex)
    {
        glDeleteTextures(1, tex->identifier);
    }


    /*==============================
        sausage64_load_staticmodel
        Generates the display lists for a
        static OpenGL model
        @param The pointer to the model data
               to generate
    ==============================*/

    void sausage64_load_staticmodel(s64ModelData* mdldata)
    {
        u32 meshcount = mdldata->meshcount;

        // Check that the model hasn't been initialized yet
        if (mdldata->meshes[0].dl->guid_mdl != 0xFFFFFFFF)
            return;

        // Enable the array client states so that the display list can be built
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);

        // Generate the buffers, and then assign them to the vert+face arrays
        for (u32 i=0; i<meshcount; i++)
        {
            s64Gfx* dl = (s64Gfx*)mdldata->meshes[i].dl;
            u32 facecount = 0, vertcount = 0;

            // Count the number of faces
            for (u32 j=0; j<dl->blockcount; j++)
            {
                vertcount += dl->renders[j].vertcount;
                facecount += dl->renders[j].facecount;
            }

            // Generate the array buffers
            glGenBuffersARB(1, &dl->guid_verts);
            glBindBufferARB(GL_ARRAY_BUFFER_ARB, dl->guid_verts);
            glBufferDataARB(GL_ARRAY_BUFFER_ARB, vertcount*sizeof(f32)*11, dl->renders[0].verts, GL_STATIC_DRAW_ARB);
            glGenBuffersARB(1, &dl->guid_faces);
            glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, dl->guid_faces);
            glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, facecount*sizeof(u16)*3, dl->renders[0].faces, GL_STATIC_DRAW_ARB);

            // Now generate the display list
            dl->guid_mdl = glGenLists(1);
            glNewList(dl->guid_mdl, GL_COMPILE);
            glBindBufferARB(GL_ARRAY_BUFFER_ARB, dl->guid_verts);
            glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, dl->guid_faces);
            for (u32 j=0; j<dl->blockcount; j++)
            {
                s64RenderBlock* render = &dl->renders[j];
                int fc = render->facecount;
                if (render->material != NULL && render->material != s64_lastmat)
                    sausage64_loadmaterial(render->material);
                s64_lastmat = render->material;
                glVertexPointer(3, GL_FLOAT, sizeof(f32)*11, (u8*)(0*sizeof(f32)));
                glTexCoordPointer(2, GL_FLOAT, sizeof(f32)*11, (u8*)(3*sizeof(f32)));
                glNormalPointer(GL_FLOAT, sizeof(f32)*11, (u8*)(5*sizeof(f32)));
                glColorPointer(3, GL_FLOAT, sizeof(f32)*11, (u8*)(8*sizeof(f32)));
                glDrawElements(GL_TRIANGLES, fc * 3, GL_UNSIGNED_SHORT, (u8*)(3*sizeof(u16)*(render->faces - dl->renders[0].faces)));
            }
            glEndList();
        }
        
        // No need for this anymore
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        s64_lastmat = NULL;
    }


    /*==============================
        sausage64_load_staticmodel
        Frees the memory used by the display 
        lists of a static OpenGL model
        @param The pointer to the model data
               to free
    ==============================*/

    void sausage64_unload_staticmodel(s64ModelData* mdldata)
    {
        u32 meshcount = mdldata->meshcount;
        for (u32 i=0; i<meshcount; i++)
        {
            s64Gfx* dl = (s64Gfx*)mdldata->meshes[i].dl;
            glDeleteBuffersARB(1, &dl->guid_verts);
            glDeleteBuffersARB(1, &dl->guid_faces);
            glDeleteLists(dl->guid_mdl, 1);
            dl->guid_mdl = 0xFFFFFFFF;
            dl->guid_verts = 0xFFFFFFFF;
            dl->guid_faces = 0xFFFFFFFF;
        }
    }
#endif


#ifndef LIBDRAGON
    /*==============================
        sausage64_gendlist
        Generate a display list from a binary block of data
        @param The data to read
        @param The display list to fill
        @param The list of verts to use
        @param The list of textures to use
    ==============================*/

    static void sausage64_gendlist(u32* data, Gfx* dlist, Vtx* verts, u32** textures)
    {
        int i;
        u32 offset = 0;
        u32 args[16];
        Gfx* dlist_original = dlist;
        while (1)
        {
            u32 datablock = data[offset++];
            switch (datablock)
            {
                case SPClearGeometryMode:
                    args[0] = data[offset++];
                    gSPClearGeometryMode(dlist++, args[0]);
                    break;
                case SPSetGeometryMode:
                    args[0] = data[offset++];
                    gSPSetGeometryMode(dlist++, args[0]);
                    break;
                case SPVertex:
                    args[0] = data[offset++];
                    gSPVertex(dlist++, verts + ((args[0] & 0xFFFF0000)>>16), (args[0] & 0x0000FF00)>>8, args[0] & 0x000000FF);
                    break;
                case SP1Triangle:
                    args[0] = data[offset++];
                    gSP1Triangle(dlist++, (args[0] & 0xFF000000)>>24, (args[0] & 0x00FF0000)>>16, (args[0] & 0x0000FF00)>>8, (args[0] & 0x000000FF));
                    break;
                case SP2Triangles:
                    args[0] = data[offset++];
                    args[1] = data[offset++];
                    gSP2Triangles(dlist++, 
                        (args[0] & 0xFF000000)>>24, (args[0] & 0x00FF0000)>>16, (args[0] & 0x0000FF00)>>8, (args[0] & 0x000000FF),
                        (args[1] & 0xFF000000)>>24, (args[1] & 0x00FF0000)>>16, (args[1] & 0x0000FF00)>>8, (args[1] & 0x000000FF)
                    );
                    break;
                case DPSetPrimColor:
                    args[0] = data[offset++];
                    args[1] = data[offset++];
                    gDPSetPrimColor(dlist++, 
                        (args[0] & 0xFFFF0000)>>16, (args[0] & 0x0000FFFF),
                        (args[1] & 0xFF000000)>>24, (args[1] & 0x00FF0000)>>16, (args[1] & 0x0000FF00)>>8, (args[1] & 0x000000FF)
                    );
                    break;
                case DPSetCombineLERP:
                    for (i=0; i<16; i++)
                        args[i] = ((u8*)(data+offset))[i];
                    offset += 4;
                    gDPSetCombineLERP_Custom(dlist++, 
                        args[0],  args[1],  args[2],  args[3], 
                        args[4],  args[5],  args[6],  args[7],
                        args[8],  args[9],  args[10], args[11], 
                        args[12], args[13], args[14], args[15]
                    );
                    break;
                case DPPipeSync:
                    gDPPipeSync(dlist++);
                    break;
                case DPSetCycleType:
                    args[0] = data[offset++];
                    gDPSetCycleType(dlist++, args[0]);
                    break;
                case DPSetRenderMode:
                    args[0] = data[offset++];
                    args[1] = data[offset++];
                    gDPSetRenderMode(dlist++, args[0], args[1]);
                    break;
                case DPSetTextureFilter:
                    args[0] = data[offset++];
                    gDPSetTextureFilter(dlist++, args[0]);
                    break;
                case DPLoadTextureBlock_4b:
                case DPLoadTextureBlock:
                    args[0] = data[offset++];
                    args[1] = data[offset++];
                    args[2] = data[offset++];
                    args[3] = data[offset++];
                    args[4] = args[0] & 0x000000FF;
                    if (textures != NULL)
                    {
                        if (args[4] == G_IM_SIZ_32b)
                        {
                            gDPLoadTextureBlock(dlist++, 
                                textures[(args[0] & 0xFFFF0000)>>16], (args[0] & 0x0000FF00)>>8, G_IM_SIZ_32b,
                                (args[1] & 0xFFFF0000)>>16, args[1] & 0x0000FFFF,
                                (args[2] & 0xFF000000)>>24, (args[2] & 0x00FF0000)>>16, (args[2] & 0x0000FF00)>>8, (args[2] & 0x000000FF),
                                (args[3] & 0xFF000000)>>24, (args[3] & 0x00FF0000)>>16, (args[3] & 0x0000FF00)>>8
                            );
                        }
                        else if (args[4] == G_IM_SIZ_16b)
                        {
                            gDPLoadTextureBlock(dlist++, 
                                textures[(args[0] & 0xFFFF0000)>>16], (args[0] & 0x0000FF00)>>8, G_IM_SIZ_16b,
                                (args[1] & 0xFFFF0000)>>16, args[1] & 0x0000FFFF,
                                (args[2] & 0xFF000000)>>24, (args[2] & 0x00FF0000)>>16, (args[2] & 0x0000FF00)>>8, (args[2] & 0x000000FF),
                                (args[3] & 0xFF000000)>>24, (args[3] & 0x00FF0000)>>16, (args[3] & 0x0000FF00)>>8
                            );
                        }
                        else if (args[4] == G_IM_SIZ_8b)
                        {
                            gDPLoadTextureBlock(dlist++, 
                                textures[(args[0] & 0xFFFF0000)>>16], (args[0] & 0x0000FF00)>>8, G_IM_SIZ_8b,
                                (args[1] & 0xFFFF0000)>>16, args[1] & 0x0000FFFF,
                                (args[2] & 0xFF000000)>>24, (args[2] & 0x00FF0000)>>16, (args[2] & 0x0000FF00)>>8, (args[2] & 0x000000FF),
                                (args[3] & 0xFF000000)>>24, (args[3] & 0x00FF0000)>>16, (args[3] & 0x0000FF00)>>8
                            );
                        }
                        else
                        {
                            gDPLoadTextureBlock_4b(dlist++, 
                                textures[(args[0] & 0xFFFF0000)>>16], (args[0] & 0x0000FF00)>>8,
                                (args[1] & 0xFFFF0000)>>16, args[1] & 0x0000FFFF,
                                (args[2] & 0xFF000000)>>24, (args[2] & 0x00FF0000)>>16, (args[2] & 0x0000FF00)>>8, (args[2] & 0x000000FF),
                                (args[3] & 0xFF000000)>>24, (args[3] & 0x00FF0000)>>16, (args[3] & 0x0000FF00)>>8
                            );
                        }
                    }
                    break;
                case SPEndDisplayList:
                    gSPEndDisplayList(dlist++);
                    return;
                default:
                    //debug_printf("Warning: Unknown DL Command with ID %d\n", datablock);
                    break;
            }
        }
    }
#endif


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
s64ModelData* sausage64_load_binarymodel(u32 romstart, u32 size, u32** textures)
#else
s64ModelData* sausage64_load_binarymodel(char* filepath, sprite_t** textures)
#endif
{
    int i;
    u8 mallocfailed = FALSE;
    #ifndef LIBDRAGON
        OSMesg   dmamsg;
        OSIoMesg iomsg;
        OSMesgQueue msgq;
        u32 left = size;
    #else
        int size;
    #endif
    u8* data;
    BinFile_Header header;
    BinFile_TOC_Meshes* toc_meshes = NULL;
    BinFile_MeshData* meshdatas = NULL;
    BinFile_TOC_Materials* toc_mats = NULL;
    BinFile_MatData* matdatas = NULL;
    BinFile_TOC_Anims* toc_anims = NULL;
    BinFile_AnimData* animdatas = NULL;
    u32 mallocsize_strings = 0, mallocsize_verts = 0, mallocsize_gfx = 0, mallocsize_keyframes = 0, mallocsize_transforms = 0;
    u32 offset_strings = 0, offset_verts = 0, offset_gfx = 0, offset_keyframes = 0, offset_transforms = 0;
    char* strings = NULL;
    #ifndef LIBDRAGON
        Vtx* verts = NULL;
    #else
        float* verts = NULL;
        u16* faces = NULL;
    #endif
    s64Gfx* dlists = NULL;
    s64Mesh* meshes = NULL;
    s64Animation* anims = NULL;
    s64KeyFrame* keyframes = NULL;
    s64Transform* transforms = NULL;
    s64ModelData* mdl = NULL;
    #ifdef LIBDRAGON
        u32 mallocsize_faces = 0, mallocsize_rbs = 0, mallocsize_texes = 0, mallocsize_primcols = 0;
        u32 offset_faces = 0, offset_rbs = 0;
        s64RenderBlock* rbs = NULL;
        s64Material* mats = NULL;
        s64Texture* texes = NULL;
        s64PrimColor* primcols = NULL;
        GLuint* texids = NULL;
    #endif
    
    // Load the asset from ROM
    #ifndef LIBDRAGON
        // Reserve some memory for the file we're about to read
        data = (u8*)memalign(16, size);
        if (data == NULL)
            return NULL;
            
        // Initialize the message queue and invalidate the data cache
        osCreateMesgQueue(&msgq, &dmamsg, 1);
        osInvalDCache((void*)data, size);

        // Read from ROM
        while (left > 0)
        {
            u32 readsize = left;
            u32 offset;
            
            // Limit the size to prevent audio stutters
            if (readsize > 16384)
                readsize = 16384;
                
            // Perform the read
            offset = size - left;
            osPiStartDma(&iomsg, OS_MESG_PRI_NORMAL, OS_READ, romstart+offset, data+offset, readsize, &msgq);
            (void)osRecvMesg(&msgq, &dmamsg, OS_MESG_BLOCK);
            left -= readsize;
        }
    #else
        data = (u8*)asset_load(filepath, &size);
        if (data == NULL)
            return NULL;
    #endif
    
    // Validate
    header.header[0] = data[0];
    header.header[1] = data[1];
    header.header[2] = data[2];
    header.header[3] = data[3];
    if (header.header[0] != 'S' || header.header[1] != '6' || header.header[2] != '4' || header.header[3] != BINARY_VERSION)
    {
        free(data);
        return NULL;
    }
    
    // Get model data
    header.count_meshes = ((u16*)data)[2];
    header.offset_meshes = ((u16*)data)[5];
    header.count_anims = ((u16*)data)[4];
    header.offset_anims = ((u32*)data)[4];
    header.count_materials = ((u16*)data)[3];
    header.offset_materials = ((u32*)data)[3];

    // Malloc temporary mesh data
    if (header.count_meshes > 0)
    {
        toc_meshes = (BinFile_TOC_Meshes*)malloc(sizeof(BinFile_TOC_Meshes)*header.count_meshes);
        meshdatas = (BinFile_MeshData*)malloc(sizeof(BinFile_MeshData)*header.count_meshes);
        if (toc_meshes == NULL || meshdatas == NULL)
            mallocfailed = TRUE;
    }

    // Malloc temporary material data
    if (header.count_materials > 0)
    {
        toc_mats = (BinFile_TOC_Materials*)malloc(sizeof(BinFile_TOC_Materials)*header.count_materials);
        matdatas = (BinFile_MatData*)malloc(sizeof(BinFile_MatData)*header.count_materials);
        if (toc_mats == NULL || matdatas == NULL)
            mallocfailed = TRUE;
    }

    // Malloc temporary animation data
    if (header.count_anims > 0)
    {
        toc_anims = (BinFile_TOC_Anims*)malloc(sizeof(BinFile_TOC_Anims)*header.count_anims);
        animdatas = (BinFile_AnimData*)malloc(sizeof(BinFile_AnimData)*header.count_anims);
        if (toc_anims == NULL || animdatas == NULL)
            mallocfailed = TRUE;
    }

    // Test that malloc succeeded
    if (mallocfailed)
    {
        free(toc_meshes);
        free(toc_mats);
        free(toc_anims);
        free(meshdatas);
        free(matdatas);
        free(animdatas);
        free(data);
        return NULL;
    }
    
    // To reduce memory fragmentation, we're going to iterate once through everything
    // to calculate the size we'll need for all the data,
    // and then the second iteration is when we'll actually populate the data structures
    for (i=0; i<header.count_meshes; i++)
    {
        #ifndef LIBDRAGON
            int toc_offset = header.offset_meshes + 0x1C*i;
            BinFile_TOC_Meshes toc_mesh = {
                *((u32*)&data[toc_offset+0*sizeof(u32)]),
                *((u32*)&data[toc_offset+1*sizeof(u32)]),
                *((u32*)&data[toc_offset+2*sizeof(u32)]),
                *((u32*)&data[toc_offset+3*sizeof(u32)]),
                0, // Unused in Libultra
                0, // Unused in Libultra
                *((u32*)&data[toc_offset+4*sizeof(u32)]),
                *((u32*)&data[toc_offset+5*sizeof(u32)]),
                *((u32*)&data[toc_offset+6*sizeof(u32)]),
            };
        #else
            int toc_offset = header.offset_meshes + 0x24*i;
            BinFile_TOC_Meshes toc_mesh = {
                *((u32*)&data[toc_offset+0*sizeof(u32)]),
                *((u32*)&data[toc_offset+1*sizeof(u32)]),
                *((u32*)&data[toc_offset+2*sizeof(u32)]),
                *((u32*)&data[toc_offset+3*sizeof(u32)]),
                *((u32*)&data[toc_offset+4*sizeof(u32)]),
                *((u32*)&data[toc_offset+5*sizeof(u32)]),
                *((u32*)&data[toc_offset+6*sizeof(u32)]),
                *((u32*)&data[toc_offset+7*sizeof(u32)]),
                *((u32*)&data[toc_offset+8*sizeof(u32)]),
            };
        #endif
        BinFile_MeshData meshdata = {
            *((u16*)&data[toc_mesh.meshdata_offset]),
            data[toc_mesh.meshdata_offset+2],
            (char*)&data[toc_mesh.meshdata_offset+3]
        };
        mallocsize_strings += strlen(meshdata.name)+1;
        #ifndef LIBDRAGON
            mallocsize_verts += toc_mesh.vertdata_size/sizeof(Vtx);
            mallocsize_gfx += toc_mesh.dldata_slotcount;
        #else
            mallocsize_verts += toc_mesh.vertdata_size/(sizeof(f32)*11);
            mallocsize_faces += toc_mesh.facedata_size/(sizeof(u16)*3);
            mallocsize_gfx += 1;
            mallocsize_rbs += toc_mesh.dldata_slotcount;
        #endif
        
        // Copy the data
        toc_meshes[i] = toc_mesh;
        meshdatas[i] = meshdata;
    }
    for (i=0; i<header.count_materials; i++)
    {
        int toc_offset = header.offset_materials + 0x10*i;
        BinFile_TOC_Materials toc_mat = {
            *((u32*)&data[toc_offset+0*sizeof(u32)]),
            *((u32*)&data[toc_offset+1*sizeof(u32)]),
            *((u32*)&data[toc_offset+2*sizeof(u32)]),
            *((u32*)&data[toc_offset+3*sizeof(u32)]),
        };
        BinFile_MatData matdata = {
            *((u8*)&data[toc_mat.matdata_offset+0]),
            *((u8*)&data[toc_mat.matdata_offset+1]),
            *((u8*)&data[toc_mat.matdata_offset+2]),
            *((u8*)&data[toc_mat.matdata_offset+3]),
            *((u8*)&data[toc_mat.matdata_offset+4]),
            *((u8*)&data[toc_mat.matdata_offset+5]),
            ((char*)&data[toc_mat.matdata_offset+6]),
        };
        #ifdef LIBDRAGON
            switch (matdata.type)
            {
                case TYPE_TEXTURE: mallocsize_texes++; break;
                case TYPE_PRIMCOL: mallocsize_primcols++; break;
            }
        #endif
        
        // Copy the data
        toc_mats[i] = toc_mat;
        matdatas[i] = matdata;
    }
    for (i=0; i<header.count_anims; i++)
    {
        int toc_offset = header.offset_anims + 0x10*i;
        BinFile_TOC_Anims toc_anim = {
            *((u32*)&data[toc_offset+0*sizeof(u32)]),
            *((u32*)&data[toc_offset+1*sizeof(u32)]),
            *((u32*)&data[toc_offset+2*sizeof(u32)]),
            *((u32*)&data[toc_offset+3*sizeof(u32)]),
        };
        BinFile_AnimData animdata = {
            *((u32*)&data[toc_anim.animdata_offset]),
            (u16*)&data[toc_anim.animdata_offset+2*sizeof(u16)]
        };
        animdata.name = (((char*)(animdata.kfindices)) + animdata.kfcount*sizeof(u16));
        mallocsize_strings += strlen(animdata.name)+1;
        mallocsize_keyframes += animdata.kfcount;
        mallocsize_transforms += animdata.kfcount*header.count_meshes;
        
        // Copy the data
        toc_anims[i] = toc_anim;
        animdatas[i] = animdata;
    }
    
    // Perform the mallocs for the data we're actually going to need
    mdl = (s64ModelData*)malloc(sizeof(s64ModelData));
    strings = (char*)malloc(sizeof(char)*mallocsize_strings);
    if (mdl == NULL || strings == NULL)
        mallocfailed = TRUE;

    // Malloc mesh data
    if (header.count_meshes > 0)
    {
        meshes = (s64Mesh*)malloc(sizeof(s64Mesh)*header.count_meshes);
        #ifndef LIBDRAGON
            verts = (Vtx*)malloc(sizeof(Vtx)*mallocsize_verts);
        #else
            verts = (f32*)malloc(sizeof(f32)*mallocsize_verts*11);
            faces = (u16*)malloc(sizeof(u16)*mallocsize_faces*3);
            rbs = (s64RenderBlock*)malloc(sizeof(s64RenderBlock)*mallocsize_rbs);
            if (faces == NULL || rbs == NULL)
                mallocfailed = TRUE;
        #endif
        dlists = (s64Gfx*)malloc(sizeof(s64Gfx)*mallocsize_gfx);
        if (meshes == NULL || verts == NULL || dlists == NULL)
            mallocfailed = TRUE;
    }

    // Malloc material data
    #ifdef LIBDRAGON
        if (header.count_materials > 0)
        {
            mats = (s64Material*)malloc(sizeof(s64Material)*header.count_materials);
            if (mats == NULL)
                mallocfailed = TRUE;
            if (mallocsize_texes > 0)
            {
                texes = (s64Texture*)malloc(sizeof(s64Texture)*mallocsize_texes);
                texids = (GLuint*)malloc(sizeof(GLuint)*mallocsize_texes);
                if (texes == NULL || texids == NULL)
                    mallocfailed = TRUE;
            }
            if (mallocsize_primcols > 0)
            {
                primcols = (s64PrimColor*)malloc(sizeof(s64PrimColor)*mallocsize_primcols);
                if (primcols == NULL)
                    mallocfailed = TRUE;
            }
        }
    #endif

    // Malloc animation data
    if (header.count_anims > 0)
    {
        anims = (s64Animation*)malloc(sizeof(s64Animation)*header.count_anims);
        keyframes = (s64KeyFrame*)malloc(sizeof(s64KeyFrame)*mallocsize_keyframes);
        transforms = (s64Transform*)malloc(sizeof(s64Transform)*mallocsize_transforms);
        if (anims == NULL || keyframes == NULL || transforms == NULL)
            mallocfailed = TRUE;
    }

    // Test that malloc succeeded
    if (mallocfailed)
    {
        free(mdl);
        free(meshes);
        free(strings);
        free(verts);
        #ifdef LIBDRAGON
            free(faces);
            free(rbs);
            free(mats);
            free(texes);
            free(texids);
            free(primcols);
        #endif
        free(dlists);
        free(anims);
        free(keyframes);
        free(transforms);
        free(toc_meshes);
        free(toc_mats);
        free(toc_anims);
        free(meshdatas);
        free(matdatas);
        free(animdatas);
        free(data);
        return NULL;
    }
    // Now we will actually pull data from the binary file and copy it over to our s64 data structs
    for (i=0; i<header.count_meshes; i++)
    {
        // Copy the s64Mesh
        *(u32*)&meshes[i].is_billboard = meshdatas[i].is_billboard;
        *(s32*)&meshes[i].parent = meshdatas[i].parent;
        meshes[i].name = strings+offset_strings;
        strcpy(strings+offset_strings, meshdatas[i].name);
        meshes[i].dl = &dlists[offset_gfx];

        #ifndef LIBDRAGON
            // Copy the vertex data
            // It's aligned by design (Thanks SGI!), so we can just memcpy
            memcpy(&verts[offset_verts], &data[toc_meshes[i].vertdata_offset], toc_meshes[i].vertdata_size);
            
            // Generate the display list
            sausage64_gendlist((u32*)(&data[toc_meshes[i].dldata_offset]), &dlists[offset_gfx], &verts[offset_verts], textures);
            
            // Increment pointers
            offset_verts += toc_meshes[i].vertdata_size/sizeof(Vtx);
            offset_gfx += toc_meshes[i].dldata_slotcount;
        #else
            // Copy the vertex and face data
            memcpy(&verts[offset_verts], &data[toc_meshes[i].vertdata_offset], toc_meshes[i].vertdata_size);
            memcpy(&faces[offset_faces], &data[toc_meshes[i].facedata_offset], toc_meshes[i].facedata_size);

            // Copy the s64Gfx data
            dlists[offset_gfx].blockcount = toc_meshes[i].dldata_slotcount;
            dlists[offset_gfx].guid_mdl = 0xFFFFFFFF;
            dlists[offset_gfx].guid_verts = 0xFFFFFFFF;
            dlists[offset_gfx].guid_faces = 0xFFFFFFFF;
            dlists[offset_gfx].renders = &rbs[offset_rbs];
            meshes[i].dl = &dlists[offset_gfx];

            // Copy the render block data
            for (int j=0; j<toc_meshes[i].dldata_slotcount; j++)
            {
                int curoffset = toc_meshes[i].dldata_offset + j*0xC;
                int matid = *((u32*)&data[curoffset + 2*sizeof(u32)]); 
                rbs[offset_rbs + j].vertcount = *((u16*)&data[curoffset + 0*sizeof(u16)]);
                rbs[offset_rbs + j].verts     = (f32(*)[11])(&verts[offset_verts] + (*((u16*)&data[curoffset + 1*sizeof(u16)]))*11);
                rbs[offset_rbs + j].facecount = *((u16*)&data[curoffset + 2*sizeof(u16)]);
                rbs[offset_rbs + j].faces     = (u16(*)[3])(&faces[offset_faces] + (*((u16*)&data[curoffset + 3*sizeof(u16)]))*3);
                if (matid == -1)
                    rbs[offset_rbs + j].material = NULL;
                else
                    rbs[offset_rbs + j].material = &mats[matid];
            }

            offset_verts += toc_meshes[i].vertdata_size/(sizeof(f32));
            offset_faces += toc_meshes[i].facedata_size/(sizeof(u16));
            offset_rbs += toc_meshes[i].dldata_slotcount;
            offset_gfx += 1;
        #endif
        offset_strings += strlen(meshes[i].name)+1;
    }
    #ifdef LIBDRAGON
        mallocsize_texes = 0;
        mallocsize_primcols = 0;
        for (i=0; i<header.count_materials; i++)
        {
            mats[i].type = matdatas[i].type;
            mats[i].lighting = matdatas[i].lighting;
            mats[i].cullfront = matdatas[i].cullfront;
            mats[i].cullback = matdatas[i].cullback;
            mats[i].smooth = matdatas[i].smooth;
            mats[i].depthtest = matdatas[i].depthtest;
            switch (mats[i].type)
            {
                case TYPE_TEXTURE:
                    mats[i].data = &texes[mallocsize_texes];
                    texids[mallocsize_texes] = 0xFFFFFFFF;
                    texes[mallocsize_texes].identifier = &texids[mallocsize_texes];
                    texes[mallocsize_texes].w = *(u32*)&data[toc_mats[i].material_offset + 0];
                    texes[mallocsize_texes].h = *(u32*)&data[toc_mats[i].material_offset + 4];
                    texes[mallocsize_texes].filter = *(u32*)&data[toc_mats[i].material_offset + 8];
                    texes[mallocsize_texes].wraps = *(u16*)&data[toc_mats[i].material_offset + 12];
                    texes[mallocsize_texes].wrapt = *(u16*)&data[toc_mats[i].material_offset + 14];
                    sausage64_load_texture(&texes[mallocsize_texes], textures[mallocsize_texes]);
                    mallocsize_texes++;
                    break;
                case TYPE_PRIMCOL:
                    mats[i].data = &primcols[mallocsize_primcols];
                    primcols[mallocsize_primcols].r = data[toc_mats[i].material_offset + 0];
                    primcols[mallocsize_primcols].g = data[toc_mats[i].material_offset + 1];
                    primcols[mallocsize_primcols].b = data[toc_mats[i].material_offset + 2];
                    primcols[mallocsize_primcols].a = data[toc_mats[i].material_offset + 3];
                    mallocsize_primcols++;
                    break;
                default: break;
            }
        }
    #endif
    for (i=0; i<header.count_anims; i++)
    {
        int j;
        
        // Copy the s64Animation
        anims[i].name = strings+offset_strings;
        strcpy(strings+offset_strings, animdatas[i].name);
        *(u32*)&anims[i].keyframecount = animdatas[i].kfcount;
        anims[i].keyframes = &keyframes[offset_keyframes];
        
        // Copy the s64KeyFrame
        for (j=0; j<animdatas[i].kfcount; j++)
        {
            *(u32*)&keyframes[offset_keyframes + j].framenumber = animdatas[i].kfindices[j];
            keyframes[offset_keyframes + j].framedata = &transforms[offset_transforms+j*header.count_meshes];
        }
        
        // Memcpy the s64Transforms
        memcpy(&transforms[offset_transforms], &data[toc_anims[i].kfdata_offset], toc_anims[i].kfdata_size);
        
        // Increment pointers
        offset_strings += strlen(anims[i].name)+1;
        offset_keyframes += animdatas[i].kfcount;
        offset_transforms += header.count_meshes*animdatas[i].kfcount;
    }
    
    // Populate the model data struct
    *(u16*)&mdl->meshcount = header.count_meshes;
    *(u16*)&mdl->animcount = header.count_anims;
    mdl->meshes = meshes;
    mdl->anims = anims;
    #ifndef LIBDRAGON
        mdl->_vtxcleanup = verts;
    #else
        mdl->_matscleanup = mats;
        mdl->_matscount = header.count_materials;
    #endif

    // Initialize the display lists for Libdragon
    #ifdef LIBDRAGON
        sausage64_load_staticmodel(mdl);
    #endif
    
    // Finish by cleaning up memory we used temporarily and returning the model data pointer
    if (header.count_meshes > 0)
    {
        free(toc_meshes);
        free(meshdatas);
    }
    if (header.count_materials > 0)
    {
        free(toc_mats);
        free(matdatas);
    }
    if (header.count_anims > 0)
    {
        free(toc_anims);
        free(animdatas);
    }
    free(data);
    return mdl;
}


/*==============================
    sausage64_unload_binarymodel
    Free the memory used by a dynamically loaded binary model
    @param  The model to free
==============================*/

void sausage64_unload_binarymodel(s64ModelData* mdl)
{
    // Because all the data is malloc'd sequentially, to free, we just need to free the first instance of everything
    if (mdl->meshcount > 0)
    {
        #ifdef LIBDRAGON
            int i;
            s64Texture* firsttex = NULL;
            s64PrimColor* firstprimcol = NULL;
            for (i=0; i<mdl->_matscount; i++)
            {
                s64Material* mat = &mdl->_matscleanup[i];
                if (mat != NULL)
                {
                    switch (mat->type)
                    {
                        case TYPE_TEXTURE: 
                            if (firsttex == NULL) 
                                firsttex = (s64Texture*)mat->data;
                            sausage64_unload_texture((s64Texture*)mat->data);
                            break;
                        case TYPE_PRIMCOL: 
                            if (firstprimcol == NULL) 
                                firstprimcol = (s64PrimColor*)mat->data;
                            break;
                        default: break;
                    }
                }
            }
            free(firsttex->identifier);
            free(firsttex);
            free(firstprimcol);
            free(mdl->_matscleanup);
            sausage64_unload_staticmodel(mdl);
            free(mdl->meshes[0].dl->renders[0].verts);
            free(mdl->meshes[0].dl->renders[0].faces);
            free(mdl->meshes[0].dl->renders);
        #else
            free(mdl->_vtxcleanup);
        #endif
        free((char*)mdl->meshes[0].name);
        free((s64Gfx*)mdl->meshes[0].dl);
        free((s64Mesh*)mdl->meshes);
    }
    if (mdl->animcount > 0)
    {
        free((s64Transform*)mdl->anims[0].keyframes[0].framedata);
        free((s64KeyFrame*)mdl->anims[0].keyframes);
        free((s64Animation*)mdl->anims);
    }
    free(mdl);
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
    mdl->rendercount = 1;
    mdl->predraw = NULL;
    mdl->postdraw = NULL;
    mdl->animcallback = NULL;
    mdl->mdldata = mdldata;

    // Set the the first animation if it exists, otherwise set the animation to NULL
    if (mdldata->animcount > 0)
        mdl->curanim.animdata = &mdldata->anims[0];
    else
        mdl->curanim.animdata = NULL;
    mdl->curanim.curtick = 0;
    mdl->curanim.curkeyframe = 0;
    
    // No animation to blend with at initialization
    mdl->blendanim.animdata = NULL;
    mdl->blendanim.curtick = 0;
    mdl->blendanim.curkeyframe = 0;
    mdl->blendticks = 0;
    mdl->blendticks_left = 0;

    // Allocate space for the transform helper
    mdl->transforms = (s64FrameTransform*)calloc(sizeof(s64FrameTransform)*mdldata->meshcount, 1);
    if (mdl->transforms == NULL)
    {
        free(mdl);
        return NULL;
    }

    // Allocate space for the model matrices in Libultra
    #ifndef LIBDRAGON
        mdl->matrix = (Mtx*)malloc(sizeof(Mtx)*1); // TODO: Handle frame buffering properly. Will require a better API
        if (mdl->matrix == NULL)
        {
            free(mdl->transforms);
            free(mdl);
            return NULL;
        }
    #endif
    return mdl;
}


/*==============================
    sausage64_set_predrawfunc
    Set a function that gets called before any mesh is rendered
    @param The model helper pointer
    @param The pre draw function
==============================*/

inline void sausage64_set_predrawfunc(s64ModelHelper* mdl, u8 (*predraw)(u16))
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


#ifndef LIBDRAGON
    
    /*==============================
        sausage64_set_camera
        Sets the camera for Sausage64 to use for billboarding
        @param The view matrix
        @param The projection matrix
    ==============================*/

    void sausage64_set_camera(Mtx* view, Mtx* projection)
    {
        guMtxL2F(s64_viewmat, view);
        guMtxL2F(s64_projmat, projection);
    }
#else
    
    /*==============================
        sausage64_set_camera
        Sets the camera for Sausage64 to use for billboarding
        @param The location of the camera, relative to the model's root
    ==============================*/

    void sausage64_set_camera(f32 campos[3])
    {
        s64_campos[0] = campos[0];
        s64_campos[1] = campos[1];
        s64_campos[2] = campos[2];
    }
#endif


/*==============================
    sausage64_update_animplay
    Updates the animation keyframe based on the animation 
    tick
    @param The animplay pointer
==============================*/

static void sausage64_update_animplay(s64AnimPlay* playing)
{
    const s64Animation* anim = playing->animdata;
    const f32 curtick = playing->curtick;
    u32 curkf_index = playing->curkeyframe;
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
                playing->curkeyframe = curkf_index;
                return;
            }
        }
        while (1);
    }
}


/*==============================
    sausage64_advance_animplay
    Advances the animation player after a tick has occurred
    @param The model helper pointer
    @param The amount to increase the animation tick by
==============================*/

static void sausage64_advance_animplay(s64ModelHelper* mdl, s64AnimPlay* playing, f32 tickamount)
{
    int rollover = 0;
    const int animlength = playing->animdata->keyframes[playing->animdata->keyframecount-1].framenumber;
    playing->curtick += tickamount;
    
    // Check for rollover
    if (playing->curtick >= animlength)
        rollover = 1;
    else if (playing->curtick <= 0)
        rollover = -1;
    
    // If the animation ended, call the callback function and roll the tick value over
    if (rollover)
    {
        f32 division;
    
        // Execute the animation end callback function
        if (mdl->animcallback != NULL)
            mdl->animcallback(playing->animdata - &mdl->mdldata->anims[0]);
        
        // If looping is disabled, then stop
        if (!mdl->loop)
        {
            if (rollover > 0)
            {
                playing->curtick = (f32)animlength;
                playing->curkeyframe = playing->animdata->keyframecount-1;
            }
            else
            {
                playing->curtick = 0;
                playing->curkeyframe = 0;
            }
            return;
        }
        
        // Calculate the correct tick
        division = playing->curtick/((f32)animlength);
        if (rollover > 0)
            playing->curtick = (division - ((int)division))*((f32)animlength);
        else
            playing->curtick = (1+(division - ((int)division)))*((f32)animlength);
    }

    // Update the animation
    if (playing->animdata->keyframecount > 0)
        sausage64_update_animplay(playing);
}


/*==============================
    sausage64_advance_anim
    Advances the animation tick by the given amount
    @param The model helper pointer
    @param The amount to increase the animation tick by
==============================*/

void sausage64_advance_anim(s64ModelHelper* mdl, f32 tickamount)
{    
    sausage64_advance_animplay(mdl, &mdl->curanim, tickamount);
    if (mdl->blendticks_left > 0)
    {
        mdl->blendticks_left -= tickamount;
        if (mdl->blendticks_left > 0)
            sausage64_advance_animplay(mdl, &mdl->blendanim, tickamount);
    }
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
    const s64Animation* animdata = &mdl->mdldata->anims[anim];
    s64AnimPlay* const playing = &mdl->curanim;
    playing->animdata = animdata;
    playing->curkeyframe = 0;
    playing->curtick = 0;
    mdl->blendticks_left = 0;
    mdl->blendticks = 0;
    if (animdata->keyframecount > 0)
        sausage64_update_animplay(&mdl->curanim);
}


/*==============================
    sausage64_set_anim_blend
    Sets an animation on the model with blending. Does not perform 
    error checking if an invalid animation is given.
    @param The model helper pointer
    @param The ANIMATION_* macro to set
    @param The amount of ticks to blend the animation over
==============================*/

void sausage64_set_anim_blend(s64ModelHelper* mdl, u16 anim, f32 ticks)
{
    if (mdl->curanim.animdata != NULL)
        mdl->blendanim = mdl->curanim;
    sausage64_set_anim(mdl, anim);
    mdl->blendticks_left = ticks;
    mdl->blendticks = ticks;
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
            const GLfloat diffuse[] = {(f32)col->r/255.0f, (f32)col->g/255.0f, (f32)col->b/255.0f, 1.0f};
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
    sausage64_calcanimlerp
    Calculates the lerp value based on the current animation
    @param  A pointer to the animation player to use
    @return The lerp amount
==============================*/

static f32 sausage64_calcanimlerp(const s64AnimPlay* playing)
{
    const s64Animation* anim = playing->animdata;
    const s64KeyFrame* ckframe = &anim->keyframes[playing->curkeyframe];
    const s64KeyFrame* nkframe = &anim->keyframes[(playing->curkeyframe+1)%anim->keyframecount];

    // Prevent division by zero when calculating the lerp amount
    if (nkframe->framenumber - ckframe->framenumber != 0)
        return ((f32)(playing->curtick - ckframe->framenumber))/((f32)(nkframe->framenumber - ckframe->framenumber));
    return 0;
}


/*==============================
    sausage64_calcanimtransforms
    Calculates the transform of a mesh based on the animation
    @param A pointer to the model helper to use
    @param The mesh to calculate the transforms of
    @param The lerp amount
    @param The lerp amount for the animation blending
==============================*/

static void sausage64_calcanimtransforms(s64ModelHelper* mdl, const u16 mesh, f32 l, f32 bl)
{
    const s64AnimPlay* playing = &mdl->curanim;
    
    // Prevent these calculations from being performed again
    if (mdl->transforms[mesh].rendercount == mdl->rendercount)
        return;
    mdl->transforms[mesh].rendercount = mdl->rendercount;

    // Calculate current animation transforms
    if (playing->animdata != NULL)
    {    
        const s64Animation* curanim = playing->animdata;
        s64Transform* fdata = &mdl->transforms[mesh].data;
        const s64Transform* cfdata = &curanim->keyframes[playing->curkeyframe].framedata[mesh];
        
        // Calculate animation lerp
        if (mdl->interpolate)
        {
            const s64Transform* nfdata = &curanim->keyframes[(playing->curkeyframe+1)%curanim->keyframecount].framedata[mesh];
            
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
    
    // Blend the anim transforms with another animation
    if (mdl->blendticks_left > 0 && mdl->interpolate)
    {
        const s64AnimPlay* blending = &mdl->blendanim;
        const s64Animation* blendanim = blending->animdata;
        s64Transform* fdata = &mdl->transforms[mesh].data;
        const s64Transform* cfdata = &blendanim->keyframes[blending->curkeyframe].framedata[mesh];
        const s64Transform* nfdata = &blendanim->keyframes[(blending->curkeyframe+1)%blendanim->keyframecount].framedata[mesh];
        const f32 blendlerp = mdl->blendticks_left/mdl->blendticks;
        
        fdata->pos[0] = s64lerp(fdata->pos[0], s64lerp(cfdata->pos[0], nfdata->pos[0], bl), blendlerp);
        fdata->pos[1] = s64lerp(fdata->pos[1], s64lerp(cfdata->pos[1], nfdata->pos[1], bl), blendlerp);
        fdata->pos[2] = s64lerp(fdata->pos[2], s64lerp(cfdata->pos[2], nfdata->pos[2], bl), blendlerp);
        if (!mdl->mdldata->meshes[mesh].is_billboard)
        {
            s64Quat qf = {fdata->rot[0], fdata->rot[1], fdata->rot[2], fdata->rot[3]};
            s64Quat q =  {cfdata->rot[0], cfdata->rot[1], cfdata->rot[2], cfdata->rot[3]};
            s64Quat qn = {nfdata->rot[0], nfdata->rot[1], nfdata->rot[2], nfdata->rot[3]};
            q = s64slerp(qf, s64slerp(q, qn, bl), blendlerp);
            fdata->rot[0] = q.w;
            fdata->rot[1] = q.x;
            fdata->rot[2] = q.y;
            fdata->rot[3] = q.z;
        }
        fdata->scale[0] = s64lerp(fdata->scale[0], s64lerp(cfdata->scale[0], nfdata->scale[0], bl), blendlerp);
        fdata->scale[1] = s64lerp(fdata->scale[1], s64lerp(cfdata->scale[1], nfdata->scale[1], bl), blendlerp);
        fdata->scale[2] = s64lerp(fdata->scale[2], s64lerp(cfdata->scale[2], nfdata->scale[2], bl), blendlerp);
    }
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
    f32 l = sausage64_calcanimlerp(&mdl->curanim);
    f32 bl = 0;
    if (mdl->blendticks_left > 0)
        bl = sausage64_calcanimlerp(&mdl->blendanim);
    sausage64_calcanimtransforms(mdl, mesh, l, bl);
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
    f32 l = sausage64_calcanimlerp(&mdl->curanim);
    f32 bl = 0;
    if (mdl->blendticks_left > 0)
        bl = sausage64_calcanimlerp(&mdl->blendanim);
    
    // First, ensure that the transforms for this mesh has been calculated
    sausage64_calcanimtransforms(mdl, mesh, l, bl);
    
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
        const s64ModelData* mdata = mdl->mdldata;
        s64Quat rotdiff = s64quat_difference(q, qt);
        for (i=0; i<mdata->meshcount; i++)  
        {
            f32 root_offset[3];
            f32 root_offset_new[3];
            s64Transform* trans_child;
            s64Quat rot;
            if (mdata->meshes[i].parent != mesh)
                continue;
            sausage64_calcanimtransforms(mdl, i, l, bl);
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
    static inline void sausage64_drawpart(Gfx** glistp, s64ModelHelper* helper, u16 mesh)
    {
        f32 helper1[4][4];
        f32 helper2[4][4];
        s64Transform* fdata = &helper->transforms[mesh].data;
        
        // Combine the translation and scale matrix
        guTranslateF(helper1, fdata->pos[0], fdata->pos[1], fdata->pos[2]);
        guScaleF(helper2, fdata->scale[0], fdata->scale[1], fdata->scale[2]);
        guMtxCatF(helper2, helper1, helper1);
        
        // Combine the rotation matrix
        if (!helper->mdldata->meshes[mesh].is_billboard)
        {
            s64Quat q = {fdata->rot[0], fdata->rot[1], fdata->rot[2], fdata->rot[3]};
            s64quat_to_mtx(q, helper2);
        }
        else
            s64calc_billboard(helper2);
        guMtxCatF(helper2, helper1, helper1);
        
        // Draw the body part
        guMtxF2L(helper1, &helper->matrix[mesh]);
        gSPMatrix((*glistp)++, OS_K0_TO_PHYSICAL(&helper->matrix[mesh]), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
        gSPDisplayList((*glistp)++, helper->mdldata->meshes[mesh].dl);
        gSPPopMatrix((*glistp)++, G_MTX_MODELVIEW);
    }
#else
    static inline void sausage64_drawpart(const s64Gfx* dl, s64ModelHelper* mdl, u16 mesh)
    {
        f32 helper1[4][4];
        s64Transform* fdata = &mdl->transforms[mesh].data;
    
        // Push a new matrix
        glPushMatrix();
        
        // Combine the translation and scale matrix
        glTranslatef(fdata->pos[0], fdata->pos[1], fdata->pos[2]);
        glScalef(fdata->scale[0], fdata->scale[1], fdata->scale[2]);

        // Combine the rotation matrix
        if (mdl->mdldata->meshes[mesh].is_billboard)
            s64calc_billboard(helper1, mdl, mesh);
        s64Quat q = {fdata->rot[0], fdata->rot[1], fdata->rot[2], fdata->rot[3]};
        s64quat_to_mtx(q, helper1);
        glMultMatrixf(&helper1[0][0]);

        // Draw the body part
        glCallList(dl->guid_mdl);
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
        f32 l = 0, bl = 0;
        const s64ModelData* mdata = mdl->mdldata;
        const u16 mcount = mdata->meshcount;
        const s64Animation* anim = mdl->curanim.animdata;
    
        // If we have a valid animation, get the lerp value
        if (anim != NULL)
        {
            l = sausage64_calcanimlerp(&mdl->curanim);
            if (mdl->blendticks_left > 0)
                bl = sausage64_calcanimlerp(&mdl->blendanim);
        }
        
        // Iterate through each mesh
        for (i=0; i<mcount; i++)
        {
            // Call the pre draw function
            if (mdl->predraw != NULL)
                if (!mdl->predraw(i))
                    continue;
            
            // Draw this part of the model
            if (anim != NULL)
            {
                sausage64_calcanimtransforms(mdl, i, l, bl);
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
        f32 l = 0, bl = 0;
        const s64ModelData* mdata = mdl->mdldata;
        const u16 mcount = mdata->meshcount;
        const s64Animation* anim = mdl->curanim.animdata;

        // Initialize OpenGL state
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
    
        // If we have a valid animation, get the lerp value
        if (anim != NULL)
        {
            l = sausage64_calcanimlerp(&mdl->curanim);
            if (mdl->blendticks_left > 0)
                bl = sausage64_calcanimlerp(&mdl->blendanim);
        }
        
        // Iterate through each mesh
        for (i=0; i<mcount; i++)
        {
            const s64Gfx* dl = mdl->mdldata->meshes[i].dl;
            
            // Call the pre draw function
            if (mdl->predraw != NULL)
                if (!mdl->predraw(i))
                    continue;
            
            // Draw this part of the model
            if (anim != NULL)
            {
                sausage64_calcanimtransforms(mdl, i, l, bl);
                sausage64_drawpart(dl, mdl, i);
            }
            else
                glCallList(dl->guid_mdl);
        
            // Call the post draw function
            if (mdl->postdraw != NULL)
                mdl->postdraw(i);
        }

        // Increment the render count for transform calculations
        mdl->rendercount++;

        // Remove the last material to prevent the material state from getting stuck
        s64_lastmat = NULL;
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