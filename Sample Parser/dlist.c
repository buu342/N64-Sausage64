/***************************************************************
                            dlist.c
                             
Constructs a display list string for outputting later.
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "main.h"
#include "texture.h"
#include "mesh.h"


/*********************************
              Macros
*********************************/

#define STRBUF_SIZE 512


/*==============================
    construct_dl
    Constructs a display list and stores it
    in a temporary file
==============================*/

void construct_dl()
{
    FILE* fp;
    char strbuff[STRBUF_SIZE];
    n64Texture* lastTexture = NULL;
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
        vertindex = 0;
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
                        fprintf(fp, "    gsDPSetCycleType(%s),\n", tex->cycle);
                        pipesync = TRUE;
                    }
                    
                    // Check for different render mode
                    if (lastTexture == NULL || strcmp(tex->rendermode1, lastTexture->rendermode1) != 0 || strcmp(tex->rendermode2, lastTexture->rendermode2) != 0)
                    {
                        fprintf(fp, "    gsDPSetRenderMode(%s, %s),\n", tex->rendermode1, tex->rendermode2);
                        pipesync = TRUE;
                    }
                    
                    // Check for different combine mode
                    if (lastTexture == NULL || strcmp(tex->combinemode1, lastTexture->combinemode1) != 0 || strcmp(tex->combinemode2, lastTexture->combinemode2) != 0)
                    {
                        fprintf(fp, "    gsDPSetCombineMode(%s, %s),\n", tex->combinemode1, tex->combinemode2);
                        pipesync = TRUE;
                    }
                    
                    // Check for different texture filter
                    if (lastTexture == NULL || strcmp(tex->texfilter, lastTexture->texfilter) != 0)
                    {
                        fprintf(fp, "    gsDPSetTextureFilter(%s),\n", tex->texfilter);
                        pipesync = TRUE;
                    }
                    
                    // Check for different geometry mode
                    if (lastTexture != NULL)
                    {
                        int flagcountold = 0, flagcountnew = 0;
                        for (i=0; i<MAXGEOFLAGS; i++)
                        {
                            int j;
                            bool hasthisflag = FALSE;
                            
                            // Skip empty flags
                            if (tex->geomode[i][0] == '\0')
                                continue;
                                
                            flagcountnew++;
                            flagcountold = 0;
                                
                            // Look through all the old texture's flags
                            for (j=0; j<MAXGEOFLAGS; j++)
                            {
                                if (lastTexture->geomode[j][0] == '\0')
                                    continue;
                                flagcountold++;
                                if (!strcmp(tex->geomode[i], lastTexture->geomode[j]))
                                {
                                    hasthisflag = TRUE;
                                    break;
                                }
                            }
                            
                            // If a flagchange was detected, the display list must be updated
                            if (!hasthisflag)
                            {
                                changedgeo = TRUE;
                                break;
                            }
                        }
                        
                        // If the number of flags changed, then we need to update the display list
                        if (flagcountold != flagcountnew)
                            changedgeo = TRUE;
                    }
                    else
                        changedgeo = TRUE;
                        
                    // If a geometry mode flag changed, then update the display list
                    if (changedgeo)
                    {
                        bool appendline = FALSE;
                    
                        // TODO: Smartly omit display list commands based on what changed
                        fprintf(fp, "    gsSPClearGeometryMode(0xFFFFFFFF),\n");
                        fprintf(fp, "    gsSPSetGeometryMode(");
                        for (i=0; i<MAXGEOFLAGS; i++)
                        {
                            if (tex->geomode[i][0] == '\0')
                                continue;
                            if (appendline)
                            {
                                fprintf(fp, " | ");
                                appendline = FALSE;
                            }
                            fprintf(fp, "%s", tex->geomode[i]);
                            appendline = TRUE;
                        }
                        fprintf(fp, "),\n");
                    }
                    
                    // Load the texture if it wasn't marked as DONTLOAD
                    if (!tex->dontload)
                    {
                        if (tex->type == TYPE_TEXTURE)
                        {
                            if (!strcmp(tex->data.image.colsize, "G_IM_SIZ_4b"))
                            {
                                fprintf(fp, "    gsDPLoadTextureBlock_4b(%s, %s, %d, %d, 0, %s, %s, %d, %d, G_TX_NOLOD, G_TX_NOLOD),\n",
                                    tex->name, tex->data.image.coltype, tex->data.image.w, tex->data.image.h, 
                                    tex->data.image.texmodes, tex->data.image.texmodet, nearest_pow2(tex->data.image.w), nearest_pow2(tex->data.image.h)
                                );
                            }
                            else
                            {
                                fprintf(fp, "    gsDPLoadTextureBlock(%s, %s, %s, %d, %d, 0, %s, %s, %d, %d, G_TX_NOLOD, G_TX_NOLOD),\n",
                                    tex->name, tex->data.image.coltype, tex->data.image.colsize, tex->data.image.w, tex->data.image.h, 
                                    tex->data.image.texmodes, tex->data.image.texmodet, nearest_pow2(tex->data.image.w), nearest_pow2(tex->data.image.h)
                                );
                            }
                            pipesync = TRUE;
                        }
                        else if (tex->type == TYPE_PRIMCOL)
                            fprintf(fp, "    gsDPSetPrimColor(0, 0, %d, %d, %d, 255),\n", tex->data.color.r, tex->data.color.g, tex->data.color.b);
                    }
                    
                    // Call a pipesync if needed
                    if (pipesync)
                        fprintf(fp, "    gsDPPipeSync(),\n");

                    // Update the last texture
                    lastTexture = tex;
                }

                // Load a new vertex block if iut hasn't been
                if (!loadedverts)
                {
                    fprintf(fp, "    gsSPVertex(vtx_%s", global_modelname);
                    if (ismultimesh)
                        fprintf(fp, "_%s", mesh->name);
                    fprintf(fp, "+%d, %d, 0),\n", vertindex, vcache->verts.size);
                    vertindex += vcache->verts.size;
                    loadedverts = TRUE;
                }
                
                // If we can, dump a 2Tri, otherwise dump a single triangle
                if (!global_no2tri && facenode->next != NULL && ((s64Face*)facenode->next->data)->texture == lastTexture)
                {
                    s64Face* prevface = face;
                    facenode = facenode->next;
                    face = (s64Face*)facenode->data;
                    fprintf(fp, "    gsSP2Triangles(%d, %d, %d, 0, %d, %d, %d, 0),\n", 
                        list_index_from_data(&vcache->verts, prevface->verts[0]), 
                        list_index_from_data(&vcache->verts, prevface->verts[1]), 
                        list_index_from_data(&vcache->verts, prevface->verts[2]), 
                        list_index_from_data(&vcache->verts, face->verts[0]), 
                        list_index_from_data(&vcache->verts, face->verts[1]), 
                        list_index_from_data(&vcache->verts, face->verts[2])
                    );
                }
                else
                    fprintf(fp, "    gsSP1Triangle(%d, %d, %d, 0),\n", 
                        list_index_from_data(&vcache->verts, face->verts[0]), 
                        list_index_from_data(&vcache->verts, face->verts[1]), 
                        list_index_from_data(&vcache->verts, face->verts[2])
                    );

                prevfacenode = facenode;
            }
            
            // Newline if we have another vertex block to load
            if (vcachenode->next != NULL)
                fprintf(fp, "\n");
        }
        fprintf(fp, "    gsSPEndDisplayList(),\n};\n\n");
    }
    
    // State we finished
    if (!global_quiet) printf("Finish building display lists\n");
    fclose(fp);
}