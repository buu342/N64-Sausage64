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
        
    // Vertex data header
    fprintf(fp, "\n/*********************************\n"
                "              Models\n"
                "*********************************/\n\n"
    );
        
    // Iterate through all the meshes
    for (listNode* meshnode = list_meshes.head; meshnode != NULL; meshnode = meshnode->next)
    {
        int vertindex = 0;
        s64Mesh* mesh = (s64Mesh*)meshnode->data;
        
        // Cycle through the vertex cache list and dump the vertices
        fprintf(fp, "static float vtx_%s", global_modelname);
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
                fprintf(fp, "    {%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f}, /* %d */\n", 
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
        fprintf(fp, "static unsigned short ind_%s", global_modelname);
        if (ismultimesh)
            fprintf(fp, "_%s", mesh->name);
        fprintf(fp, "[][3] = {\n");
        vertindex = 0;
        for (listNode* vcachenode = mesh->vertcache.head; vcachenode != NULL; vcachenode = vcachenode->next)
        {
            vertCache* vcache = (vertCache*)vcachenode->data;
            
            // Cycle through all the faces
            for (listNode* vertnode = vcache->faces.head; vertnode != NULL; vertnode = vertnode->next)
            {
                s64Face* face = (s64Face*)vertnode->data;
                
                // Dump the face data
                fprintf(fp, "    {%u, %u, %u}, /* %d */\n", 
                    list_index_from_data(&vcache->verts, face->verts[0]), 
                    list_index_from_data(&vcache->verts, face->verts[1]), 
                    list_index_from_data(&vcache->verts, face->verts[2]),
                    vertindex++
                );
            }
        }
        fprintf(fp, "};\n\n");
    
        // Finally, cycle through the vertex cache list again, but now create OpenGL commands
        fprintf(fp, "static void gfx_%s", global_modelname);
        if (ismultimesh)
            fprintf(fp, "_%s", mesh->name);
        fprintf(fp, "()\n{\n");
        vertindex = 0;
        //for (listNode* vcachenode = mesh->vertcache.head; vcachenode != NULL; vcachenode = vcachenode->next)
        //{
            //vertCache* vcache = (vertCache*)vcachenode->data;
            //listNode* prevfacenode = vcache->faces.head;
            
            fprintf(fp, "    glGenBuffersARB(2, buffers);\n");
            fprintf(fp, "    glBindBufferARB(GL_ARRAY_BUFFER_ARB, buffers[0]);\n");
            fprintf(fp, "    glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW_ARB);\n");
            fprintf(fp, "    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, buffers[1]);\n");
            fprintf(fp, "    glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW_ARB);\n");
            
            fprintf(fp, "    glEnable(GL_LIGHTING);\n");
            fprintf(fp, "    glEnable(GL_DEPTH_TEST);\n");
            fprintf(fp, "    glEnable(GL_CULL_FACE);\n");
            fprintf(fp, "    glEnable(G_CULL_BACK);\n");
            fprintf(fp, "    glEnable(G_SHADING_SMOOTH);\n");
            fprintf(fp, "    glEnable(GL_COLOR_MATERIAL);\n");
            fprintf(fp, "    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);\n");
            
            fprintf(fp, "    glEnableClientState(GL_VERTEX_ARRAY);\n");
            fprintf(fp, "    glEnableClientState(GL_TEXTURE_COORD_ARRAY);\n");
            fprintf(fp, "    glEnableClientState(GL_NORMAL_ARRAY);\n");
            fprintf(fp, "    glEnableClientState(GL_COLOR_ARRAY);\n");

            fprintf(fp, "    glVertexPointer(3, GL_FLOAT, 11, NULL + 0*sizeof(float));\n");
            fprintf(fp, "    glTexCoordPointer(2, GL_FLOAT, 11, NULL + 3*sizeof(float));\n");
            fprintf(fp, "    glNormalPointer(GL_FLOAT, 11, NULL + 5*sizeof(float));\n");
            fprintf(fp, "    glColorPointer(3, GL_FLOAT, 11, NULL + 8*sizeof(float));\n");
            
            fprintf(fp, "    glDrawElements(GL_TRIANGLES, %d, GL_UNSIGNED_SHORT, 0);\n", mesh->faces.size);
            
            // Newline if we have another vertex block to load
        //    if (vcachenode->next != NULL)
        //        fprintf(fp, "\n");
        //}
        fprintf(fp, "}\n\n");
    }
    
    // State we finished
    if (!global_quiet) printf("Finish building display lists\n");
    fclose(fp);
}