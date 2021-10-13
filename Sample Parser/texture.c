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
    n64Texture* tex = (n64Texture*)malloc(sizeof(n64Texture));
    if (tex == NULL)
        terminate("Error: Unable to allocate memory for texture object\n");
    tex->name = (char*)calloc(strlen(name)+1, 1);
    if (tex->name == NULL)
        terminate("Error: Unable to allocate memory for texture name\n");
    
    // Store the data in the newly created texture struct
    tex->type = TYPE_TEXTURE;
    strcpy(tex->name, name);
    tex->data.image.w = w;
    tex->data.image.h = h;
    
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
    n64Texture* tex = (n64Texture*)malloc(sizeof(n64Texture));
    if (tex == NULL)
        terminate("Error: Unable to allocate memory for primitive color object\n");
    tex->name = (char*)calloc(strlen(name)+1, 1);
    if (tex->name == NULL)
        terminate("Error: Unable to allocate memory for primitive color name\n");
    
    // Store the data in the newly created texture struct
    tex->type = TYPE_PRIMCOL;
    strcpy(tex->name, name);
    tex->data.color.r = r;
    tex->data.color.g = g;
    tex->data.color.b = b;
    
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
    listNode* curnode = list_textures.head;
    
    // Iterate through the textures list
    while (curnode != NULL)
    {
        n64Texture* ctex = (n64Texture*)curnode->data;
        
        // If the name matches, return the texture struct pointer
        if (!strcmp(ctex->name, name))
            return ctex;
            
        // Otherwise, go to the next node
        curnode = curnode->next;
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
        char *name;
        texType type;
        short w, h;
        color r, g, b;
        
        // Read a string from the text file
        fgets(strbuf, STRBUFF_SIZE, fp);
        
        // Get our starting data
        name = strtok(strbuf, " ");
        type = (texType)atoi(strtok(NULL, " "));
        
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
            default:
                sprintf(strbuf, "Error parsing texture file -> Unknown texture type '%d' in texture '%s'\n", name, type);
                terminate(strbuf);
        }
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
    char strbuf[STRBUFF_SIZE];
    texType type;
    short w, h;
    color r, g, b;
    n64Texture* tex;
    
    // Request the texture type
    printf("New texture '%s' found, please specify the following:\n", name);
    printf("\tTexture type (0 = image, 1 = primitive color): ");
    scanf("%d", (int*)&type);
    
    // Create the texture from the type
    switch (type)
    {
        case TYPE_TEXTURE:
            printf("\tTexture Width: ");
            scanf("%d", &w);
            printf("\tTexture Height: ");
            scanf("%d", &h);
            tex = add_texture(name, w, h);
            if (!global_quiet) printf("Added texture '%s'\n", name);
            break;
        case TYPE_PRIMCOL:
            printf("\tPrimitve Red: ");
            scanf("%d", &r);
            printf("\tPrimitve Green: ");
            scanf("%d", &g);
            printf("\tPrimitve Blue: ");
            scanf("%d", &b);
            tex = add_primcol(name, r, g, b);
            if (!global_quiet) printf("Added primitive color '%s'\n", name);
            break;
        default:
            sprintf(strbuf, "Error: Unknown texture type '%d'\n", (int)type);
            terminate(strbuf);
    }
    return tex;
}