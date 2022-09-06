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

#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wunused-result" // TODO: Implement proper error checking

n64Texture* parse_textures(FILE* fp)
{
    n64Texture* tex;
    if (!global_quiet) printf("Parsing textures file\n");
    
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
        strbuf[strcspn(strbuf, "\r\n")] = 0;
        
        // Get our starting data
        name = strtok(strbuf, " ");
        tok = strtok(NULL, " ");
        
        // Ignore empty lines
        if (tok == NULL)
            continue;
        
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
                if (!global_quiet) printf("    Added texture '%s'\n", name);
                break;
            case TYPE_PRIMCOL:
                r = (color)atoi(strtok(NULL, " "));
                g = (color)atoi(strtok(NULL, " "));
                b = (color)atoi(strtok(NULL, " "));
                tex = add_primcol(name, r, g, b);
                if (!global_quiet) printf("    Added primitive color '%s'\n", name);
                break;
            case TYPE_OMIT:
                tex = (n64Texture*)calloc(1, sizeof(n64Texture));
                if (tex == NULL)
                    terminate("Error: Unable to allocate memory for none type texture\n");
                tex->name = (char*)calloc(1, strlen(name)+1);
                if (tex->name == NULL)
                    terminate("Error: Unable to allocate memory for none texture name\n");
                strcpy(tex->name, name);
                tex->type = TYPE_OMIT;
                list_append(&list_textures, tex);
                break;
        }
        
        // Parse the rest of the arguments
        for (tok = strtok(NULL, " \n\r"); tok != NULL; tok = strtok(NULL, " \n\r"))
            tex_setflag(tex, tok);
    }
    
    // Close the file, as it's done being parsed
    if (!global_quiet) printf("Finished parsing textures file\n");
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
            scanf("%d", (int*)&w); fflush(stdin);
            printf("\tTexture Height: ");
            scanf("%d", (int*)&h); fflush(stdin);
            tex = add_texture(name, w, h);
            printf("\tTexture flags (separate by spaces): ");
            fgets(strbuf, STRBUFF_SIZE, stdin);
            for (tok = strtok(strbuf, " \n\r"); tok != NULL; tok = strtok(NULL, " \n\r"))
                tex_setflag(tex, tok);
            if (!global_quiet) printf("Added texture '%s'\n", name);
            break;
        case TYPE_PRIMCOL:
            printf("\tPrimitve Red: ");
            scanf("%d", (int*)&r); fflush(stdin);
            printf("\tPrimitve Green: ");
            scanf("%d", (int*)&g); fflush(stdin);
            printf("\tPrimitve Blue: ");
            scanf("%d", (int*)&b); fflush(stdin);
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
            strcpy(tex->name, name);
            tex->type = TYPE_OMIT;
            list_append(&list_textures, tex);
            if (!global_quiet) printf("Omitting texture '%s'\n", name);
            return tex;
        default:
            sprintf(strbuf, "Error: Unknown texture type '%d'\n", (int)type);
            terminate(strbuf);
    }
    return tex;
}
#pragma GCC diagnostic pop

/*==============================
    tex_setflag
    Allows you to modify a texture flag
    @param The texture to change
    @param The flag to set
==============================*/

void tex_setflag(n64Texture* tex, char* flag)
{
    // Static variables to keep track of stuff between sucessive calls to this function
    char* copy;
    static n64Texture* lasttex = NULL;
    static bool render2 = FALSE;
    static bool combine2 = FALSE;
    static bool texmode2 = FALSE;
    
    // Make a copy of the string
    copy = (char*)calloc(1, strlen(flag)+1);
    if (copy == NULL)
        terminate("Error: Unable to allocate memory for flag name copy\n");
    strcpy(copy, flag);
    
    // If our texture changed, reset the last flags
    if (lasttex != tex)
    {
        render2 = FALSE;
        combine2 = FALSE;
        texmode2 = FALSE;
        lasttex = tex;
    }
    
    // Set the texture flags
    if (!strncmp(copy, DONTLOAD, sizeof(DONTLOAD)-1))
    {
        tex->dontload = TRUE;
    }
    else if (!strncmp(copy, LOADFIRST, sizeof(LOADFIRST)-1))
    {
        tex->loadfirst = TRUE;
    }
    else if (!strncmp(copy, G_CYC_, sizeof(G_CYC_)-1))
    {
        tex->cycle = copy;
    }
    else if (!strncmp(copy, G_TF_, sizeof(G_TF_)-1))
    {
        tex->texfilter = copy;
    }
    else if (!strncmp(copy, G_CC_, sizeof(G_CC_)-1))
    {
        if (!combine2)
        {
            tex->combinemode1 = copy;
            combine2 = TRUE;
        }
        else
            tex->combinemode2 = copy;
    }
    else if (!strncmp(copy, G_RM_, sizeof(G_RM_)-1))
    {
        if (!render2)
        {
            tex->rendermode1 = copy;
            render2 = TRUE;
        }
        else
            tex->rendermode2 = copy;
    }
    else if (!strncmp(copy, G_IM_FMT_, sizeof(G_IM_FMT_)-1))
    {
        if (tex->type != TYPE_TEXTURE)
            terminate("Error: Attempted to set image format on something that isn't a texture!\n");
        tex->data.image.coltype = copy;
    }
    else if (!strncmp(copy, G_IM_SIZ_, sizeof(G_IM_SIZ_)-1))
    {
        if (tex->type != TYPE_TEXTURE)
            terminate("Error: Attempted to set image bit size on something that isn't a texture!\n");
        tex->data.image.colsize = copy;
    }
    else if (!strncmp(copy, G_TX_, sizeof(G_TX_)-1))
    {
        if (tex->type != TYPE_TEXTURE)
            terminate("Error: Attempted to set texture mode on something that isn't a texture!\n");
        if (!texmode2)
        {
            tex->data.image.texmodes = copy;
            texmode2 = TRUE;
        }
        else
            tex->data.image.texmodet = copy;
    }
    else
    {
        int i;
        if (copy[0] != '!') // Set texture flag
        {
            for (i=0; i<MAXGEOFLAGS; i++)
            {
                if (tex->geomode[i][0] == '\0')
                {
                    strcpy(tex->geomode[i], copy);
                    break;
                }
            }
        }
        else // Remove texture flag
        {
            for (i=0; i<MAXGEOFLAGS; i++)
            {
                if (!strcmp(copy+1, tex->geomode[i]))
                {
                    memset(tex->geomode[i], 0, GEOFLAGSIZE);
                    break;
                }
            }        
        }
    }
}


/*==============================
    tex_hasgeoflag
    Checks if a texture has a geometry flag
    @param The texture to change
    @param The flag to set
==============================*/

bool tex_hasgeoflag(n64Texture* tex, char* flag)
{
    int i;
    for (i=0; i<MAXGEOFLAGS; i++)
        if (!strcmp(flag, tex->geomode[i]))
            return TRUE;
    return FALSE;
}