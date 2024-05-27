/***************************************************************
                            dlist.c
                             
Constructs a display list string for outputting later.
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "main.h"
#include "texture.h"
#include "mesh.h"
#include "dlist.h"


/*********************************
              Macros
*********************************/

#define STRBUF_SIZE 512

#define generate(c, ...) (generator(c, commands_f3dex2[c].argcount, ##__VA_ARGS__))


/*********************************
              Globals
*********************************/

// List of valid display list commands as of F3DEX2
const DListCommand commands_f3dex2[] = {
    {DPFillRectangle, "DPFillRectangle", 4, 1},
    {DPScisFillRectangle, "DPScisFillRectangle", 4, 1},
    {DPFullSync, "DPFullSync", 0, 1},
    {DPLoadSync, "DPLoadSync", 0, 1},
    {DPTileSync, "DPTileSync", 0, 1},
    {DPPipeSync, "DPPipeSync", 0, 1},
    {DPLoadTLUT_pal16, "DPLoadTLUT_pal16", 2, 6},
    {DPLoadTLUT_pal256, "DPLoadTLUT_pal256", 1, 6},
    {DPLoadTextureBlock, "DPLoadTextureBlock", 12, 7},
    {DPLoadTextureBlock_4b, "DPLoadTextureBlock_4b", 11, 7},
    {DPLoadTextureTile, "DPLoadTextureTile", 16, 7},
    {DPLoadTextureTile_4b, "DPLoadTextureTile_4b", 15, 7},
    {DPLoadBlock, "DPLoadBlock", 5, 1},
    {DPNoOp, "DPNoOp", 0, 1},
    {DPNoOpTag, "DPNoOpTag", 1, 1},
    {DPPipelineMode, "DPPipelineMode", 1, 1},
    {DPSetBlendColor, "DPSetBlendColor", 4, 1},
    {DPSetEnvColor, "DPSetEnvColor", 4, 1},
    {DPSetFillColor, "DPSetFillColor", 1, 1},
    {DPSetFogColor, "DPSetFogColor", 4, 1},
    {DPSetPrimColor, "DPSetPrimColor", 6, 1},
    {DPSetColorImage, "DPSetColorImage", 4, 1},
    {DPSetDepthImage, "DPSetDepthImage", 1, 1},
    {DPSetTextureImage, "DPSetTextureImage", 4, 1},
    {DPSetHilite1Tile, "DPSetHilite1Tile", 4, 1},
    {DPSetHilite2Tile, "DPSetHilite2Tile", 4, 1},
    {DPSetAlphaCompare, "DPSetAlphaCompare", 1, 1},
    {DPSetAlphaDither, "DPSetAlphaDither", 1, 1},
    {DPSetColorDither, "DPSetColorDither", 1, 1},
    {DPSetCombineMode, "DPSetCombineMode", 2, 1},
    {DPSetCombineLERP, "DPSetCombineLERP", 16, 1},
    {DPSetConvert, "DPSetConvert", 6, 1},
    {DPSetTextureConvert, "DPSetTextureConvert", 1, 1},
    {DPSetCycleType, "DPSetCycleType", 1, 1},
    {DPSetDepthSource, "DPSetDepthSource", 1, 1},
    {DPSetCombineKey, "DPSetCombineKey", 1, 1},
    {DPSetKeyGB, "DPSetKeyGB", 6, 1},
    {DPSetKeyR, "DPSetKeyR", 3, 1},
    {DPSetPrimDepth, "DPSetPrimDepth", 2, 1},
    {DPSetRenderMode, "DPSetRenderMode", 2, 1},
    {DPSetScissor, "DPSetScissor", 5, 1},
    {DPSetTextureDetail, "DPSetTextureDetail", 1, 1},
    {DPSetTextureFilter, "DPSetTextureFilter", 1, 1},
    {DPSetTextureLOD, "DPSetTextureLOD", 1, 1},
    {DPSetTextureLUT, "DPSetTextureLUT", 1, 1},
    {DPSetTexturePersp, "DPSetTexturePersp", 1, 1},
    {DPSetTile, "DPSetTile", 12, 1},
    {DPSetTileSize, "DPSetTileSize", 5, 1},
    {SP1Triangle, "SP1Triangle", 4, 1},
    {SP2Triangles, "SP2Triangles", 8, 1},
    {SPBranchLessZ, "SPBranchLessZ", 6, 2},
    {SPBranchLessZrg, "SPBranchLessZrg", 8, 2},
    {SPBranchList, "SPBranchList", 1, 1},
    {SPClipRatio, "SPClipRatio", 1, 4},
    {SPCullDisplayList, "SPCullDisplayList", 2, 1},
    {SPDisplayList, "SPDisplayList", 1, 1},
    {SPEndDisplayList, "SPEndDisplayList", 0, 1},
    {SPFogPosition, "SPFogPosition", 2, 1},
    {SPForceMatrix, "SPForceMatrix", 1, 2},
    {SPSetGeometryMode, "SPSetGeometryMode", 1, 1},
    {SPClearGeometryMode, "SPClearGeometryMode", 1, 1},
    {SPInsertMatrix, "SPInsertMatrix", 2, 1},
    {SPLine3D, "SPLine3D", 3, 1},
    {SPLineW3D, "SPLineW3D", 4, 1},
    {SPLoadUcode, "SPLoadUcode", 2, 2},
    {SPLoadUcodeL, "SPLoadUcodeL", 1, 2},
    {SPLookAt, "SPLookAt", 1, 2},
    {SPMatrix, "SPMatrix", 2, 1},
    {SPModifyVertex, "SPModifyVertex", 3, 1},
    {SPPerspNormalize, "SPPerspNormalize", 1, 1},
    {SPPopMatrix, "SPPopMatrix", 1, 1},
    {SPSegment, "SPSegment", 2, 1},
    {SPSetLights0, "SPSetLights0", 1, 3},
    {SPSetLights1, "SPSetLights1", 1, 3},
    {SPSetLights2, "SPSetLights2", 1, 4},
    {SPSetLights3, "SPSetLights3", 1, 5},
    {SPSetLights4, "SPSetLights4", 1, 6},
    {SPSetLights5, "SPSetLights5", 1, 7},
    {SPSetLights6, "SPSetLights6", 1, 8},
    {SPSetLights7, "SPSetLights7", 1, 9},
    {SPSetStatus, "SPSetStatus", 2, 1},
    {SPNumLights, "SPNumLights", 1, 1},
    {SPLight, "SPLight", 2, 1},
    {SPLightColor, "SPLightColor", 2, 2},
    {SPTexture, "SPTexture", 5, 1},
    {SPTextureRectangle, "SPTextureRectangle", 9, 3},
    {SPScisTextureRectangle, "SPScisTextureRectangle", 9, 3},
    {SPTextureRectangleFlip, "SPTextureRectangleFlip", 9, 3},
    {SPVertex, "SPVertex", 3, 1},
    {SPViewport, "SPViewport", 1, 1},
    {SPBgRectCopy, "SPBgRectCopy", 1, 1},
    {SPBgRect1Cyc, "SPBgRect1Cyc", 1, 1},
    {SPObjRectangle, "SPObjRectangle", 1, 1},
    {SPObjRectangleR, "SPObjRectangleR", 1, 1},
    {SPObjSprite, "SPObjSprite", 1, 1},
    {SPObjMatrix, "SPObjMatrix", 1, 1},
    {SPObjSubMatrix, "SPObjSubMatrix", 1, 1},
    {SPObjRenderMode, "SPObjRenderMode", 1, 1},
    {SPObjLoadTxtr, "SPObjLoadTxtr", 1, 1},
    {SPObjLoadTxRect, "SPObjLoadTxRect", 1, 1},
    {SPObjLoadTxRectR, "SPObjLoadTxRectR", 1, 1},
    {SPObjLoadTxSprite, "SPObjLoadTxSprite", 1, 1},
    {SPSelectDL, "SPSelectDL", 4, 2},
    {SPSelectBranchDL, "SPSelectBranchDL", 4, 2},
};

// Global parsing state
n64Texture* lastTexture = NULL;


/*==============================
    mallocstring
    Creates a newly malloc'ed copy of a string
    @param   The string to create
    @returns A malloc'ed copy
==============================*/

static char* mallocstring(char* str)
{
    char* ret = (char*)malloc(sizeof(char)*(strlen(str)+1));
    strcpy(ret, str);
    return ret;
}


/*==============================
    _dlist_commandstring
    Creates a static display list command
    string from a dlist command.
    Don't use directly, use the macro
    @param   The display list command name
    @param   The number of arguments
    @param   Variable arguments
    @returns A malloc'ed string
==============================*/

static void* _dlist_commandstring(DListCName c, int size, ...)
{
    va_list args;
    char strbuff[STRBUF_SIZE];
    va_start(args, size);
    sprintf(strbuff, "    gs%s(", commands_f3dex2[c].name);
    for (int i=0; i<size; i++)
    {
        strcat(strbuff, va_arg(args, char*));
        if (i+1 != size)
            strcat(strbuff, ", ");
    }
    strcat(strbuff, "),\n");
    va_end(args);
    return mallocstring(strbuff);
}

static void* _dlist_commandbinary(DListCName c, int size, ...)
{
    return 0;
}

/*==============================
    dlist_frommesh
    Constructs a display list from a single mesh
    @param   The mesh to build a DL of
    @param   Whether the DL should be binary
    @returns A linked list with the DL data
==============================*/

linkedList* dlist_frommesh(s64Mesh* mesh, char isbinary)
{
    char strbuff[STRBUF_SIZE];
    linkedList* out = list_new();
    bool ismultimesh = (list_meshes.size > 1);
    int vertindex = 0;
    if (out == NULL)
        terminate("Error: Unable to malloc for output list\n");
    void* (*generator)(DListCName c, int size, ...);

    // Select the generation function
    if (isbinary)
        generator = &_dlist_commandbinary;
    else
        generator = &_dlist_commandstring;

    // Loop through the vertex caches
    for (listNode* vcachenode = mesh->vertcache.head; vcachenode != NULL; vcachenode = vcachenode->next)
    {
        vertCache* vcache = (vertCache*)vcachenode->data;
        listNode* prevfacenode = vcache->faces.head;
        bool loadedverts = FALSE;
        
        // Cycle through all the faces
        for (listNode* facenode = vcache->faces.head; facenode != NULL; facenode = facenode->next)
        {
            s64Face* face = (s64Face*)facenode->data;
            n64Texture* tex = face->texture;
            
            // If we want to skip the initial display list setup, then change the value of our last texture to skip the next if statement
            if (lastTexture == NULL && !global_initialload)
                lastTexture = tex;
        
            // If a texture change was detected, load the new texture data
            if (lastTexture != tex && tex->type != TYPE_OMIT)
            {
                int i;
                bool pipesync = FALSE;
                bool changedgeo = FALSE;
                
                // Check for different cycle type
                if (lastTexture == NULL || strcmp(tex->cycle, lastTexture->cycle) != 0)
                {
                    list_append(out, generate(DPSetCycleType, tex->cycle));
                    pipesync = TRUE;
                }
                
                // Check for different render mode
                if (lastTexture == NULL || strcmp(tex->rendermode1, lastTexture->rendermode1) != 0 || strcmp(tex->rendermode2, lastTexture->rendermode2) != 0)
                {
                    list_append(out, generate(DPSetRenderMode, tex->rendermode1, tex->rendermode2));
                    pipesync = TRUE;
                }
                
                // Check for different combine mode
                if (lastTexture == NULL || strcmp(tex->combinemode1, lastTexture->combinemode1) != 0 || strcmp(tex->combinemode2, lastTexture->combinemode2) != 0)
                {
                    list_append(out, generate(DPSetCombineMode, tex->combinemode1, tex->combinemode2));
                    pipesync = TRUE;
                }
                
                // Check for different texture filter
                if (lastTexture == NULL || strcmp(tex->texfilter, lastTexture->texfilter) != 0)
                {
                    list_append(out, generate(DPSetTextureFilter, tex->texfilter));
                    pipesync = TRUE;
                }
                
                // Check for different geometry mode
                if (lastTexture != NULL)
                {
                    int flagcount_old = 0;
                    int flagcount_new = 0;
                    char* flags_old[MAXGEOFLAGS];
                    char* flags_new[MAXGEOFLAGS];

                    // Store the pointer to the flags somewhere to make the iteration easier
                    for (i=0; i<MAXGEOFLAGS; i++)
                    {
                        if (tex->geomode[i][0] != '\0')
                        {
                            flags_new[flagcount_new] = tex->geomode[i];
                            flagcount_new++;
                        }
                        if (lastTexture->geomode[i][0] != '\0')
                        {
                            flags_old[flagcount_old] = lastTexture->geomode[i];
                            flagcount_old++;
                        }
                    }

                    // Check if all the flags exist in this other texture
                    if (flagcount_new == flagcount_old)
                    {
                        int j;
                        bool hasthisflag = FALSE;
                        for (i=0; i<flagcount_new; i++)
                        {
                            for (j=0; j<flagcount_old; j++)
                            {
                                if (!strcmp(flags_new[i], flags_old[j]))
                                {
                                    hasthisflag = TRUE;
                                    break;
                                }
                            }
                            if (!hasthisflag)
                            {
                                changedgeo = TRUE;
                                break;
                            }
                        }
                    }
                    else
                        changedgeo = TRUE;
                }
                else
                    changedgeo = TRUE;
                    
                // If a geometry mode flag changed, then update the display list
                if (changedgeo)
                {
                    bool appendline = FALSE;
                
                    // TODO: Smartly omit geometry flags commands based on what changed
                    list_append(out, generate(SPClearGeometryMode, "0xFFFFFFFF"));
                    strbuff[0] = '\0';
                    for (i=0; i<MAXGEOFLAGS; i++)
                    {
                        if (tex->geomode[i][0] == '\0')
                            continue;
                        if (appendline)
                        {
                            strcat(strbuff, " | ");
                            appendline = FALSE;
                        }
                        strcat(strbuff, tex->geomode[i]);
                        appendline = TRUE;
                    }
                    list_append(out, generate(SPSetGeometryMode, strbuff));
                }
                
                // Load the texture if it wasn't marked as DONTLOAD
                if (!tex->dontload)
                {
                    char d1[32], d2[32], d3[32], d4[32];
                    if (tex->type == TYPE_TEXTURE)
                    {
                        sprintf(d1, "%d", tex->data.image.w);
                        sprintf(d2, "%d", tex->data.image.h);
                        sprintf(d3, "%d", nearest_pow2(tex->data.image.w));
                        sprintf(d4, "%d", nearest_pow2(tex->data.image.h));
                        if (!strcmp(tex->data.image.colsize, "G_IM_SIZ_4b"))
                        {
                            list_append(out, generate(DPLoadTextureBlock_4b, 
                                tex->name, tex->data.image.coltype, d1, d2, "0",
                                tex->data.image.texmodes, tex->data.image.texmodet, d3, d4, "G_TX_NOLOD", "G_TX_NOLOD")
                            );
                        }
                        else
                        {
                            list_append(out, generate(DPLoadTextureBlock, 
                                tex->name, tex->data.image.coltype, tex->data.image.colsize, d1, d2, "0",
                                tex->data.image.texmodes, tex->data.image.texmodet, d3, d4, "G_TX_NOLOD", "G_TX_NOLOD")
                            );
                        }
                        pipesync = TRUE;
                    }
                    else if (tex->type == TYPE_PRIMCOL)
                    {
                        sprintf(d1, "%d", tex->data.color.r);
                        sprintf(d2, "%d", tex->data.color.g);
                        sprintf(d3, "%d", tex->data.color.b);
                        list_append(out, generate(DPSetPrimColor, "0", "0", d1, d2, d3, "255"));
                    }
                }
                
                // Call a pipesync if needed
                if (pipesync)
                    list_append(out, generate(DPPipeSync));

                // Update the last texture
                lastTexture = tex;
            }

            // Load a new vertex block if it hasn't been
            if (!loadedverts)
            {
                char d2[32];
                sprintf(strbuff, "vtx_%s", global_modelname);
                if (ismultimesh)
                {
                    strcat(strbuff, "_");
                    strcat(strbuff, mesh->name);
                }
                strcat(strbuff, "+");
                sprintf(d2, "%d", vertindex);
                strcat(strbuff, d2);
                sprintf(d2, "%d", vcache->verts.size);
                list_append(out, generate(SPVertex, strbuff, d2, "0"));
                vertindex += vcache->verts.size;
                loadedverts = TRUE;
            }
            
            // If we can, dump a 2Tri, otherwise dump a single triangle
            if (!global_no2tri && facenode->next != NULL && ((s64Face*)facenode->next->data)->texture == lastTexture)
            {
                char d1[32], d2[32], d3[32], d4[32], d5[32], d6[32];
                s64Face* prevface = face;
                facenode = facenode->next;
                face = (s64Face*)facenode->data;
                sprintf(d1, "%d", list_index_from_data(&vcache->verts, prevface->verts[0]));
                sprintf(d2, "%d", list_index_from_data(&vcache->verts, prevface->verts[1]));
                sprintf(d3, "%d", list_index_from_data(&vcache->verts, prevface->verts[2]));
                sprintf(d4, "%d", list_index_from_data(&vcache->verts, face->verts[0]));
                sprintf(d5, "%d", list_index_from_data(&vcache->verts, face->verts[1]));
                sprintf(d6, "%d", list_index_from_data(&vcache->verts, face->verts[2]));
                list_append(out, generate(SP2Triangles, d1, d2, d3, "0", d4, d5, d6, "0"));
            }
            else
            {
                char d1[32], d2[32], d3[32];
                sprintf(d1, "%d", list_index_from_data(&vcache->verts, face->verts[0]));
                sprintf(d2, "%d", list_index_from_data(&vcache->verts, face->verts[1]));
                sprintf(d3, "%d", list_index_from_data(&vcache->verts, face->verts[2]));
                list_append(out, generate(SP1Triangle, d1, d2, d3, "0"));
            }

            prevfacenode = facenode;
        }
        
        // Newline if we have another vertex block to load
        if (!isbinary && vcachenode->next != NULL)
            list_append(out, mallocstring("\n"));
    }
    list_append(out, generate(SPEndDisplayList));
    return out;
}


/*==============================
    construct_dl
    Constructs a display list and stores it
    in a temporary file
    @param Whether the DL should be binary
==============================*/

void construct_dl(char isbinary)
{
    FILE* fp;
    linkedList* dl;
    char strbuff[STRBUF_SIZE];
    bool ismultimesh = (list_meshes.size > 1);
    
    // Open a temp file to write our display list to
    sprintf(strbuff, "temp_%s", global_outputname);
    fp = fopen(strbuff, "w+");
    if (fp == NULL)
        terminate("Error: Unable to open temporary file for writing\n");
    
    // Announce we're gonna construct the DL
    if (!global_quiet) printf("Constructing display lists\n");
    
    // Vertex data header
    fprintf(fp, "\n// Custom combine mode to allow mixing primitive and vertex colors\n"
                "#ifndef G_CC_PRIMLITE\n    #define G_CC_PRIMLITE SHADE,0,PRIMITIVE,0,0,0,0,PRIMITIVE\n#endif\n\n\n"
                "/*********************************\n"
                "              Models\n"
                "*********************************/\n\n"
    );
    
    // Iterate through all the meshes
    for (listNode* meshnode = list_meshes.head; meshnode != NULL; meshnode = meshnode->next)
    {
        int vertindex = 0;
        s64Mesh* mesh = (s64Mesh*)meshnode->data;
        
        // Cycle through the vertex cache list and dump the vertices
        fprintf(fp, "static Vtx vtx_%s", global_modelname);
        if (ismultimesh)
            fprintf(fp, "_%s", mesh->name);
        fprintf(fp, "[] = {\n");
        for (listNode* vcachenode = mesh->vertcache.head; vcachenode != NULL; vcachenode = vcachenode->next)
        {
            vertCache* vcache = (vertCache*)vcachenode->data;
            listNode* vertnode;
            
            // Cycle through all the verts
            for (vertnode = vcache->verts.head; vertnode != NULL; vertnode = vertnode->next)
            {
                int texturew = 0, textureh = 0;
                s64Vert* vert = (s64Vert*)vertnode->data;
                n64Texture* tex = find_texture_fromvert(&vcache->faces, vert);
                Vector3D normorcol = {0, 0, 0};
                
                // Ensure the texture is valid
                if (tex == NULL)
                    terminate("Error: Inconsistent face/vertex texture information\n");
                
                // Retrieve texture/normal/color data for this vertex
                switch (tex->type)
                {
                    case TYPE_TEXTURE:
                        // Get the texture size
                        texturew = (tex->data).image.w;
                        textureh = (tex->data).image.h;
                        
                        // Intentional fallthrough
                    case TYPE_PRIMCOL:
                        // Pick vertex normals or vertex colors, depending on the texture flag
                        if (tex_hasgeoflag(tex, "G_LIGHTING"))
                            normorcol = vector_scale(vert->normal, 127);
                        else
                            normorcol = vector_scale(vert->color, 255);
                        break;
                    case TYPE_OMIT:
                        break;
                }
                
                // Dump the vert data
                fprintf(fp, "    {%d, %d, %d, 0, %d, %d, %d, %d, %d, 255}, /* %d */\n", 
                    (int)round(vert->pos.x), (int)round(vert->pos.y), (int)round(vert->pos.z),
                    float_to_s10p5(vert->UV.x*texturew), float_to_s10p5(vert->UV.y*textureh),
                    (int)round(normorcol.x), (int)round(normorcol.y), (int)round(normorcol.z),
                    vertindex++
                );
            }
        }
        fprintf(fp, "};\n\n");
        
        // Then cycle through the vertex cache list again, but now dump the display list
        fprintf(fp, "static Gfx gfx_%s", global_modelname);
        if (ismultimesh)
            fprintf(fp, "_%s", mesh->name);
        fprintf(fp, "[] = {\n");
        dl = dlist_frommesh(mesh, 0);
        for (listNode* dlnode = dl->head; dlnode != NULL; dlnode = dlnode->next)
            fprintf(fp, "%s", (char*)dlnode->data);
        list_destroy_deep(dl);
        fprintf(fp, "};\n\n");
    }
    
    // State we finished
    if (!global_quiet) printf("Finish building display lists\n");
    fclose(fp);
}