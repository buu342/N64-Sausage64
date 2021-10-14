/***************************************************************
                           texture.c
                             
Handles the parsing and creation of textures
***************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "main.h"
#include "texture.h"

/*********************************
              Macros
*********************************/

#define STRBUFF_SIZE 512


/*********************************
             Globals
*********************************/

static char default_geoflags[10][32] = DEFAULT_GEOFLAGS;
n64Texture texture_none = {.name = "None", .type = TYPE_OMIT};


/*==============================
    add_texture
    Creates an image texture and adds it to the texture list
    @param The name of the texture
    @param The image width in texels
    @param The image height in texels
    @returns The newly created n64Texture
==============================*/

n64Texture* add_texture(char* name, short w, short h)
{
    // Allocate memory for the texture struct and string name
    n64Texture* tex = (n64Texture*)calloc(1, sizeof(n64Texture));
    if (tex == NULL)
        terminate("Error: Unable to allocate memory for texture object\n");
    tex->name = (char*)calloc(1, strlen(name)+1);
    if (tex->name == NULL)
        terminate("Error: Unable to allocate memory for texture name\n");
    
    // Store the data in the newly created texture struct
    tex->type = TYPE_TEXTURE;
    strcpy(tex->name, name);
    tex->data.image.w = w;
    tex->data.image.h = h;
    
    // Initialize the default stuff
    tex->cycle = DEFAULT_CYCLE;
    tex->texfilter = DEFAULT_TEXFILTER;
    tex->combinemode1 = DEFAULT_COMBINE1_TEX; 
    tex->combinemode2 = DEFAULT_COMBINE2_TEX;
    tex->rendermode1 = DEFAULT_RENDERMODE1;
    tex->rendermode2 = DEFAULT_RENDERMODE2;
    tex->data.image.coltype = DEFAULT_IMAGEFORMAT;
    tex->data.image.colsize = DEFAULT_IMAGESIZE;
    tex->data.image.texmodes = DEFAULT_TEXFLAGS; 
    tex->data.image.texmodet = DEFAULT_TEXFLAGT;
    memcpy(tex->geomode, default_geoflags, 10*32);
    
    // Add this texture to our texture list and return it
    list_append(&list_textures, tex);
    return tex;
}


/*==============================
    add_primcol
    Creates an primitive color texture and adds it to the texture list
    @param The name of the texture
    @param The primitve red component
    @param The primitve green component
    @param The primitve blue component
    @returns The newly created n64Texture
==============================*/

n64Texture* add_primcol(char* name, color r, color g, color b)
{
    // Allocate memory for the texture struct and string name
    n64Texture* tex = (n64Texture*)calloc(1, sizeof(n64Texture));
    if (tex == NULL)
        terminate("Error: Unable to allocate memory for primitive color object\n");
    tex->name = (char*)calloc(1, strlen(name)+1);
    if (tex->name == NULL)
        terminate("Error: Unable to allocate memory for primitive color name\n");
    
    // Store the data in the newly created texture struct
    tex->type = TYPE_PRIMCOL;
    strcpy(tex->name, name);
    tex->data.color.r = r;
    tex->data.color.g = g;
    tex->data.color.b = b;
    
    // Initialize the default stuff
    tex->cycle = DEFAULT_CYCLE;
    tex->texfilter = DEFAULT_TEXFILTER;
    tex->combinemode1 = DEFAULT_COMBINE1_PRIM; 
    tex->combinemode2 = DEFAULT_COMBINE2_PRIM;
    tex->rendermode1 = DEFAULT_RENDERMODE1;
    tex->rendermode2 = DEFAULT_RENDERMODE2;
    memcpy(tex->geomode, default_geoflags, 10*32);
    
    // Add this texture to our texture list and return it
    list_append(&list_textures, tex);
    return tex;
}


/*==============================
    find_texture
    Searches through the list of textures for a texture with the specified name
    @param The texture name
    @returns The requested n64Texture, or NULL if none was found 
==============================*/

n64Texture* find_texture(char* name)
{
    listNode* curnode;
    
    // Iterate through the textures list
    for (curnode = list_textures.head; curnode != NULL; curnode = curnode->next)
    {
        n64Texture* ctex = (n64Texture*)curnode->data;
        
        // If the name matches, return the texture struct pointer
        if (!strcmp(ctex->name, name))
            return ctex;
    }
    
    // No texture found, return NULL
    return NULL;
}


/*==============================
    parse_textures
    Reads a text file and parses texture data from it
    This function does very little error checking
    @param The pointer to the texture file's handle
    @returns A pointer to the new n64Texture 
==============================*/

n64Texture* parse_textures(FILE* fp)
{
    n64Texture* tex;
    if (!global_quiet) printf("*Parsing textures file\n");
    
    // Read the file until we reached the end
    while (!feof(fp))
    {
        char strbuf[STRBUFF_SIZE];
        char* name;
        char* tok;
        texType type = TYPE_UNKNOWN;
        short w, h;
        color r, g, b;
        
        // Read a string from the text file
        fgets(strbuf, STRBUFF_SIZE, fp);
        
        // Get our starting data
        name = strtok(strbuf, " ");
        tok = strtok(NULL, " ");
        
        // Get the texture type
        if (!strcmp(tok, TEXTURE))
            type = TYPE_TEXTURE;
        else if (!strcmp(tok, PRIMCOL))
            type = TYPE_PRIMCOL;
        else if (!strcmp(tok, OMIT))
            type = TYPE_OMIT;
        
        // If we don't know the texture type, stop
        if (type == TYPE_UNKNOWN)
        {
            char *readname, *readtype;
            readname = (char*)calloc(1, strlen(name)+1);
            readtype = (char*)calloc(1, strlen(tok)+1);
            if (readname == NULL || readtype == NULL)
                terminate("Error: Unable to allocate memory for texture error message. Oh dear...\n");
            strcpy(readname, name);
            strcpy(readtype, tok);
            memset(strbuf, 0, STRBUFF_SIZE);
            sprintf(strbuf, "Error: Unknown texture type '%s' in texture '%s'\n", readtype, readname);
            terminate(strbuf);
        }
        
        // Create the texture from the type
        switch (type)
        {
            case TYPE_TEXTURE:
                w = (short)atoi(strtok(NULL, " "));
                h = (short)atoi(strtok(NULL, " "));
                tex = add_texture(name, w, h);
                if (!global_quiet) printf("Added texture '%s'\n", name);
                break;
            case TYPE_PRIMCOL:
                r = (color)atoi(strtok(NULL, " "));
                g = (color)atoi(strtok(NULL, " "));
                b = (color)atoi(strtok(NULL, " "));
                tex = add_primcol(name, r, g, b);
                if (!global_quiet) printf("Added primitive color '%s'\n", name);
                break;
            case TYPE_OMIT:
                tex = (n64Texture*)calloc(1, sizeof(n64Texture));
                if (tex == NULL)
                    terminate("Error: Unable to allocate memory for none type texture\n");
                tex->name = (char*)calloc(1, strlen(name)+1);
                if (tex->name == NULL)
                    terminate("Error: Unable to allocate memory for none texture name\n");
                tex->type = TYPE_OMIT;
                list_append(&list_textures, tex);
                break;
        }
        
        // Parse the rest of the arguments
        for (tok = strtok(NULL, " \n\r"); tok != NULL; tok = strtok(NULL, " \n\r"))
            tex_setflag(tex, tok);
    }
    
    // Close the file, as it's done being parsed
    if (!global_quiet) printf("*Finished parsing textures file\n");
    fclose(fp);
    return tex;
}


/*==============================
    request_texture
    Requests information regarding a new texture from the user
    @param The name of the new texture
    @returns A pointer to the new n64Texture 
==============================*/

n64Texture* request_texture(char* name)
{
    char* tok;
    char strbuf[STRBUFF_SIZE];
    texType type;
    short w, h;
    color r, g, b;
    n64Texture* tex;
    
    // Request the texture type
    printf("New texture '%s' found, please specify the following:\n", name);
    printf("\tTexture type (0 = omit, 1 = texture, 2 = primitive color): ");
    scanf("%d", (int*)&type);
    
    // Create the texture from the type
    switch (type)
    {
        case TYPE_TEXTURE:
            printf("\tTexture Width: ");
            scanf("%d", &w); fflush(stdin);
            printf("\tTexture Height: ");
            scanf("%d", &h); fflush(stdin);
            tex = add_texture(name, w, h);
            printf("\tTexture flags (separate by spaces): ");
            fgets(strbuf, STRBUFF_SIZE, stdin);
            for (tok = strtok(strbuf, " \n\r"); tok != NULL; tok = strtok(NULL, " \n\r"))
                tex_setflag(tex, tok);
            if (!global_quiet) printf("Added texture '%s'\n", name);
            break;
        case TYPE_PRIMCOL:
            printf("\tPrimitve Red: ");
            scanf("%d", &r); fflush(stdin);
            printf("\tPrimitve Green: ");
            scanf("%d", &g); fflush(stdin);
            printf("\tPrimitve Blue: ");
            scanf("%d", &b); fflush(stdin);
            tex = add_primcol(name, r, g, b);
            printf("\tTexture flags (separate by spaces): ");
            fgets(strbuf, STRBUFF_SIZE, stdin);
            for (tok = strtok(strbuf, " \n\r"); tok != NULL; tok = strtok(NULL, " \n\r"))
                tex_setflag(tex, tok);
            if (!global_quiet) printf("Added primitive color '%s'\n", name);
            break;
        case TYPE_OMIT:
            tex = (n64Texture*)calloc(1, sizeof(n64Texture));
            if (tex == NULL)
                terminate("Error: Unable to allocate memory for none type texture\n");
            tex->name = (char*)calloc(1, strlen(name)+1);
            if (tex->name == NULL)
                terminate("Error: Unable to allocate memory for none texture name\n");
            tex->type = TYPE_OMIT;
            list_append(&list_textures, tex);
            return tex;
        default:
            sprintf(strbuf, "Error: Unknown texture type '%d'\n", (int)type);
            terminate(strbuf);
    }
    return tex;
}


/*==============================
    tex_setflag
    Allows you to modify a texture flag
    @param The texture to change
    @param The flag to set
==============================*/

void tex_setflag(n64Texture* tex, char* flag)
{
    printf("%s -> %s\n", tex->name, flag);
}