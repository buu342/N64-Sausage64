/***************************************************************
                            opengl.c
                             
Constructs an OpenGL command list string for outputting 
later.
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "main.h"
#include "texture.h"
#include "mesh.h"

typedef struct {
    int vertcount;
    int vertoffset;
    int facecount;
    int faceoffset;
    n64Texture* tex;
} VCacheRenderBlock;


/*********************************
              Macros
*********************************/

#define STRBUF_SIZE 512


/*==============================
    construct_opengl
    Constructs an OpenGL Command List 
    and stores it in a temporary file
==============================*/

void construct_opengl()
{
    FILE* fp;
    char strbuff[STRBUF_SIZE];
    n64Texture* lastTexture = NULL;
    bool ismultimesh = (list_meshes.size > 1);
    
    // Open a temp file to write our opengl command list to
    sprintf(strbuff, "temp_%s", global_outputname);
    fp = fopen(strbuff, "w+");
    if (fp == NULL)
        terminate("Error: Unable to open temporary file for writing\n");
        
    // Texture data header
    fprintf(fp, "\n/*********************************\n"
                "             Textures\n"
                "*********************************/\n\n"
    );
    
    // Generate the s64Material struct for each texture
    for (listNode* texnode = list_textures.head; texnode != NULL; texnode = texnode->next)
    {
        n64Texture* tex = (n64Texture*)texnode->data;
        
        // Generate the material data
        switch (tex->type)
        {
            case TYPE_OMIT:
                break;
            case TYPE_TEXTURE:
                fprintf(fp, "static s64Texture matdata_%s = {&%s};\n", tex->name, tex->name);
                break;
            case TYPE_PRIMCOL:
                fprintf(fp, "static s64Primcolor matdata_%s = {%d, %d, %d, %d};\n", tex->name, tex->data.color.r, tex->data.color.g, tex->data.color.b, tex->data.color.a);
                break;
        }
        
        // And then the material itself
        fprintf(fp, "static s64Material mat_%s = {", tex->name);
        switch (tex->type)
        {
            case TYPE_OMIT:
                fprintf(fp, "TYPE_OMIT, NULL");
                break;
            case TYPE_TEXTURE:
                fprintf(fp, "TYPE_TEXTURE, &matdata_%s, %d, %d, %d, %d, %d, %d", tex->name, 
                    tex_hasgeoflag(tex, "G_LIGHTING"), tex_hasgeoflag(tex, "G_CULL_FRONT"), tex_hasgeoflag(tex, "G_CULL_BACK"),
                    tex_hasgeoflag(tex, "G_SHADING_SMOOTH"), tex_hasgeoflag(tex, "G_ZBUFFER"), tex->dontload
                );
                break;
            case TYPE_PRIMCOL:
                fprintf(fp, "TYPE_PRIMCOL, &matdata_%s, %d, %d, %d, %d, %d, %d", tex->name, 
                    tex_hasgeoflag(tex, "G_LIGHTING"), tex_hasgeoflag(tex, "G_CULL_FRONT"), tex_hasgeoflag(tex, "G_CULL_BACK"),
                    tex_hasgeoflag(tex, "G_SHADING_SMOOTH"), tex_hasgeoflag(tex, "G_ZBUFFER"), tex->dontload
                );
                break;
        }
        fprintf(fp, "};\n\n");
    }
    
        
    // Vertex data header
    fprintf(fp, "\n/*********************************\n"
                "              Models\n"
                "*********************************/\n\n"
    );
        
    // Iterate through all the meshes
    for (listNode* meshnode = list_meshes.head; meshnode != NULL; meshnode = meshnode->next)
    {
        s64Mesh* mesh = (s64Mesh*)meshnode->data;
        int texcount = 0;
        int faceindex = 0, vertindex = 0;
        lastTexture = NULL;
        VCacheRenderBlock* lastrenderblock = NULL;
        linkedList list_vcacherender = EMPTY_LINKEDLIST;
        
        // First, cycle through the vertex cache list and register the texture switches
        for (listNode* vcachenode = mesh->vertcache.head; vcachenode != NULL; vcachenode = vcachenode->next)
        {
            vertCache* vcache = (vertCache*)vcachenode->data;
            for (listNode* vertnode = vcache->faces.head; vertnode != NULL; vertnode = vertnode->next)
            {
                s64Face* face = (s64Face*)vertnode->data;
                if (face->texture != lastTexture)
                {
                    texcount++;
                    lastTexture = face->texture;
                    VCacheRenderBlock* vcrb = calloc(1, sizeof(VCacheRenderBlock));
                    if (vcrb == NULL)
                        terminate("Error: Unable to allocate memory for VCache Render Block\n");
                    vcrb->tex = face->texture;
                    if (lastrenderblock == NULL)
                    {
                        vcrb->vertoffset = 0;
                        vcrb->faceoffset = 0;
                    }
                    else
                    {
                        vcrb->vertoffset = lastrenderblock->vertcount + lastrenderblock->vertoffset;
                        vcrb->faceoffset = lastrenderblock->facecount + lastrenderblock->faceoffset;
                    }
                    lastrenderblock = vcrb;
                    list_append(&list_vcacherender, vcrb);
                }
            }
            lastrenderblock->facecount += vcache->faces.size;
            lastrenderblock->vertcount += vcache->verts.size;
        }
        
        // Cycle through the vertex cache list and dump the vertices
        vertindex = 0;
        fprintf(fp, "static f32 vtx_%s", global_modelname);
        if (ismultimesh)
            fprintf(fp, "_%s", mesh->name);
        fprintf(fp, "[][11] = {\n");
        for (listNode* vcachenode = mesh->vertcache.head; vcachenode != NULL; vcachenode = vcachenode->next)
        {
            vertCache* vcache = (vertCache*)vcachenode->data;
            
            // Cycle through all the verts
            for (listNode* vertnode = vcache->verts.head; vertnode != NULL; vertnode = vertnode->next)
            {
                s64Vert* vert = (s64Vert*)vertnode->data;
                
                // Dump the vert data
                fprintf(fp, "    {%.4ff, %.4ff, %.4ff, %.4ff, %.4ff, %.4ff, %.4ff, %.4ff, %.4ff, %.4ff, %.4ff}, /* %d */\n", 
                    vert->pos.x, vert->pos.y, vert->pos.z,
                    vert->UV.x, vert->UV.y,
                    vert->normal.x, vert->normal.y, vert->normal.z,
                    vert->color.x, vert->color.y, vert->color.z,
                    vertindex++
                );
            }
        }
        fprintf(fp, "};\n\n");
        
        // Then cycle through the vertex cache list again, but now dump the faces
        fprintf(fp, "static u16 ind_%s", global_modelname);
        if (ismultimesh)
            fprintf(fp, "_%s", mesh->name);
        fprintf(fp, "[][3] = {\n");
        vertindex = 0;
        faceindex = 0;
        for (listNode* vcachenode = mesh->vertcache.head; vcachenode != NULL; vcachenode = vcachenode->next)
        {
            vertCache* vcache = (vertCache*)vcachenode->data;
            
            // Cycle through all the faces
            for (listNode* vertnode = vcache->faces.head; vertnode != NULL; vertnode = vertnode->next)
            {
                s64Face* face = (s64Face*)vertnode->data;
                
                // Dump the face data
                fprintf(fp, "    {%u, %u, %u}, /* %d */\n", 
                    vertindex + list_index_from_data(&vcache->verts, face->verts[0]), 
                    vertindex + list_index_from_data(&vcache->verts, face->verts[1]), 
                    vertindex + list_index_from_data(&vcache->verts, face->verts[2]),
                    faceindex++
                );
            }
            vertindex += vcache->verts.size;
        }
        fprintf(fp, "};\n\n");
        
        // Next, generate the render blocks
        fprintf(fp, "static s64RenderBlock renb_%s", global_modelname);
        if (ismultimesh)
            fprintf(fp, "_%s", mesh->name);
        fprintf(fp, "[] = {\n");
        for (listNode* vcachenode = list_vcacherender.head; vcachenode != NULL; vcachenode = vcachenode->next)
        {
            VCacheRenderBlock* vcacheb = (VCacheRenderBlock*)vcachenode->data;
            fprintf(fp, "\t{");
            fprintf(fp, "&vtx_%s", global_modelname);
            if (ismultimesh)
                fprintf(fp, "_%s", mesh->name);
            fprintf(fp, "[%d], %d, %d, ", vcacheb->vertoffset, vcacheb->vertcount, vcacheb->facecount);
            fprintf(fp, "&ind_%s", global_modelname);
            if (ismultimesh)
                fprintf(fp, "_%s", mesh->name);
            fprintf(fp, "[%d], &mat_%s},\n", vcacheb->faceoffset, vcacheb->tex->name);
        }
        fprintf(fp, "};\n\n");
        
        // Finally, generate the "Display List"
        fprintf(fp, "static s64Gfx gfx_%s", global_modelname);
        if (ismultimesh)
            fprintf(fp, "_%s", mesh->name);
        fprintf(fp, " = {%d, ", list_vcacherender.size);
        fprintf(fp, "renb_%s", global_modelname);
        if (ismultimesh)
            fprintf(fp, "_%s", mesh->name);
        fprintf(fp, "};\n\n");
        
        // Cleanup
        list_destroy_deep(&list_vcacherender);
    }
    
    // State we finished
    if (!global_quiet) printf("Finish building display lists\n");
    fclose(fp);
}