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
#include "gbi.h"

/*********************************
              Macros
*********************************/

#define STRBUF_SIZE 512

#define generate(c, ...) (generator(c, commands_f3dex2[c].argcount, ##__VA_ARGS__))


/*********************************
              Globals
*********************************/

// List of commands supported by Arabiki
const DListCName supported_binary[] = {
    DPSetCycleType,
    DPSetRenderMode,
    DPSetCombineMode,
    DPSetTextureFilter,
    SPClearGeometryMode,
    SPSetGeometryMode,
    DPLoadTextureBlock,
    DPLoadTextureBlock_4b,
    DPSetPrimColor,
    SPVertex,
    SP1Triangle,
    SP2Triangles,
    DPPipeSync,
    SPEndDisplayList
}; 

// Global parsing state
n64Texture* lastTexture = NULL;


/*==============================
    swap_endian16
    Swaps the endianess of the data (16-bit)
    @param   The data to swap the endianess of
    @returns The data with endianess swapped
==============================*/

uint16_t swap_endian16(uint16_t val)
{
    return ((val << 8) & 0xFF00) | ((val >> 8) & 0x00FF);
}


/*==============================
    swap_endian32
    Swaps the endianess of the data (32-bit)
    @param   The data to swap the endianess of
    @returns The data with endianess swapped
==============================*/

uint32_t swap_endian32(uint32_t val)
{
    return ((val << 24)) | ((val << 8) & 0x00FF0000) | ((val >> 8) & 0x0000FF00) | ((val >> 24));
}


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


/*==============================
    _dlist_commandbinary
    Creates a binary display list command
    from a dlist command.
    Don't use directly, use the macro
    @param   The display list command name
    @param   The number of arguments
    @param   Variable arguments
    @returns A malloc'ed binary block
==============================*/

static void* _dlist_commandbinary(DListCName c, int size, ...)
{
    char supported = 0;
    va_list args, args2;
    char strbuff[STRBUF_SIZE];
    int* binarydata;
    int datasize = size+1;
    va_start(args, size);
    va_copy(args2, args);

    // Check the command is supported
    for (int i=0; i<sizeof(supported_binary)/sizeof(supported_binary[0]); i++)
    {
        if (supported_binary[i] == c)
        {
            supported = 1;
            break;
        }
    }
    if (!supported)
    {
        sprintf(strbuff, "Unsupported Binary DL command %s", commands_f3dex2[c].name);
        terminate(strbuff);
    }

    // Malloc the binary data
    switch (c)
    {
        // TODO: Compact LoadTextureBlock
        // TODO: Compact SP1Triangle
        // TODO: Compact SP2Triangles
        case DPSetCombineMode: // Since CC Mode is 16 arguments (64 bytes), we're gonna compact the data as all CC mode argument numbers fit in a single byte
            datasize = 4 + 1;
            binarydata = (int*)malloc(sizeof(int)*datasize);
            if (binarydata == NULL)
                terminate("Unable to malloc binary data buffer");
            binarydata[0] = swap_endian32(DPSetCombineLERP);
            break;
        default:
            binarydata = (int*)malloc(sizeof(int)*datasize);
            if (binarydata == NULL)
                terminate("Unable to malloc binary data buffer");
            binarydata[0] = swap_endian32(c);
            break;
    }

    // Go through each argument
    for (int i=0; i<size; i++)
    {
        char* arg = va_arg(args, char*);
        bool parsed = FALSE;

        // Handle edge cases
        switch (c)
        {
            case DPLoadTextureBlock:
            case DPLoadTextureBlock_4b:
                if (i == 0) // First argument is the texture name, we just want the texture index
                {
                    binarydata[1] = swap_endian32(get_validtexindex(&list_textures, arg));
                    parsed = TRUE;
                }
                break;
            case DPSetCombineMode:
                memcpy(&binarydata[1+i*2], gbi_resolveccmode(arg), 8);
                parsed = TRUE;
                break;
            case SPVertex:
                if (i == 0) // First argument is a pointer offset, we just want the offset
                {
                    char* curchar = arg;
                    while (*curchar != '+')
                        curchar++;
                    binarydata[1] = swap_endian32(atoi(curchar+1));
                    parsed = TRUE;
                }
                break;
        }

        // Handle everything else (Macro values, ints as strings, and hex values as strings)
        if (!parsed)
        {
            int argval;
            if (strlen(arg) > 2 && arg[0] == 'G' && arg[1] == '_')
                argval = gbi_resolvemacro(arg);
            else if (strlen(arg) > 2 && arg[0] == '0' && arg[1] == 'x')
                argval = strtol(arg, NULL, 16);
            else
                argval = atoi(arg);
            binarydata[i+1] = swap_endian32(argval);
        }
    }
    va_end(args);

    // Debugging printf
    // Don't forget to get rid of args2
    printf("Turned %s with ", commands_f3dex2[c].name);
    for (int i=0; i<size; i++)
        printf("%s, ", va_arg(args2, char*));
    printf("into ");
    for (int i=0; i<datasize; i++)
        printf("%08x ", binarydata[i]);
    printf("\n");
    va_end(args2);

    return binarydata;
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
    construct_dltext
    Constructs a display list and stores it
    in a temporary file
==============================*/

void construct_dltext()
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
        dl = dlist_frommesh(mesh, FALSE);
        for (listNode* dlnode = dl->head; dlnode != NULL; dlnode = dlnode->next)
            fprintf(fp, "%s", (char*)dlnode->data);
        list_destroy_deep(dl);
        fprintf(fp, "};\n\n");
    }
    
    // State we finished
    if (!global_quiet) printf("Finish building display lists\n");
    fclose(fp);
}