/***************************************************************
                           material.c
                             
Handles the parsing and creation of materials
***************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "main.h"
#include "material.h"


/*********************************
              Macros
*********************************/

#define STRBUFF_SIZE 512


/*********************************
             Globals
*********************************/

static char default_geoflags[10][32] = DEFAULT_GEOFLAGS;
n64Material material_none = {.name = "None", .type = TYPE_OMIT};


/*==============================
    clean_stdin
    fflush(stdin) doesn't work on Linux, so this is an alternative
==============================*/

void clean_stdin()
{
    int c;
    do {
        c = getchar();
    } while (c != '\n' && c != EOF);
}


/*==============================
    add_texture
    Creates an image texture and adds it to the material list
    @param The name of the material
    @param The image width in texels
    @param The image height in texels
    @returns The newly created n64Material
==============================*/

n64Material* add_texture(char* name, short w, short h)
{
    // Allocate memory for the material struct and string name
    n64Material* mat = (n64Material*)calloc(1, sizeof(n64Material));
    if (mat == NULL)
        terminate("Error: Unable to allocate memory for texture object\n");
    mat->name = (char*)calloc(1, strlen(name)+1);
    if (mat->name == NULL)
        terminate("Error: Unable to allocate memory for texture name\n");
    
    // Store the data in the newly created texture struct
    mat->type = TYPE_TEXTURE;
    strcpy(mat->name, name);
    mat->data.image.w = w;
    mat->data.image.h = h;
    
    // Initialize the default stuff
    mat->cycle = DEFAULT_CYCLE;
    mat->texfilter = DEFAULT_TEXFILTER;
    mat->combinemode1 = DEFAULT_COMBINE1_TEX; 
    mat->combinemode2 = DEFAULT_COMBINE2_TEX;
    mat->rendermode1 = DEFAULT_RENDERMODE1;
    mat->rendermode2 = DEFAULT_RENDERMODE2;
    mat->data.image.coltype = DEFAULT_IMAGEFORMAT;
    mat->data.image.colsize = DEFAULT_IMAGESIZE;
    mat->data.image.texmodes = DEFAULT_TEXFLAGS; 
    mat->data.image.texmodet = DEFAULT_TEXFLAGT;
    memcpy(mat->geomode, default_geoflags, 10*32);
    
    // Add this texture to our material list and return it
    list_append(&list_materials, mat);
    return mat;
}


/*==============================
    add_primcol
    Creates a primitive color material and adds it to the material list
    @param The name of the material
    @param The primitve red component
    @param The primitve green component
    @param The primitve blue component
    @returns The newly created n64Material
==============================*/

n64Material* add_primcol(char* name, color r, color g, color b)
{
    // Allocate memory for the material struct and string name
    n64Material* mat = (n64Material*)calloc(1, sizeof(n64Material));
    if (mat == NULL)
        terminate("Error: Unable to allocate memory for primitive color object\n");
    mat->name = (char*)calloc(1, strlen(name)+1);
    if (mat->name == NULL)
        terminate("Error: Unable to allocate memory for primitive color name\n");
    
    // Store the data in the newly created material struct
    mat->type = TYPE_PRIMCOL;
    strcpy(mat->name, name);
    mat->data.color.r = r;
    mat->data.color.g = g;
    mat->data.color.b = b;
    
    // Initialize the default stuff
    mat->cycle = DEFAULT_CYCLE;
    mat->texfilter = DEFAULT_TEXFILTER;
    mat->combinemode1 = DEFAULT_COMBINE1_PRIM; 
    mat->combinemode2 = DEFAULT_COMBINE2_PRIM;
    mat->rendermode1 = DEFAULT_RENDERMODE1;
    mat->rendermode2 = DEFAULT_RENDERMODE2;
    memcpy(mat->geomode, default_geoflags, 10*32);
    
    // Add this material to our materials list and return it
    list_append(&list_materials, mat);
    return mat;
}


/*==============================
    find_material
    Searches through the list of materials for a material with the specified name
    @param The material name
    @returns The requested n64Material, or NULL if none was found 
==============================*/

n64Material* find_material(char* name)
{
    listNode* curnode;
    
    // Iterate through the material list
    for (curnode = list_materials.head; curnode != NULL; curnode = curnode->next)
    {
        n64Material* cmat = (n64Material*)curnode->data;
        
        // If the name matches, return the material struct pointer
        if (!strcmp(cmat->name, name))
            return cmat;
    }
    
    // No material found, return NULL
    return NULL;
}


/*==============================
    parse_materials
    Reads a text file and parses material data from it
    This function does very little error checking
    @param The pointer to the material file's handle
    @returns A pointer to the new n64Material 
==============================*/

#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wunused-result" // TODO: Implement proper error checking

n64Material* parse_materials(FILE* fp)
{
    n64Material* mat;
    if (!global_quiet) printf("Parsing materials file\n");
    
    // Read the file until we reached the end
    while (!feof(fp))
    {
        char strbuf[STRBUFF_SIZE];
        char* name;
        char* tok;
        matType type = TYPE_UNKNOWN;
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
        
        // Get the material type
        if (!strcmp(tok, TEXTURE))
            type = TYPE_TEXTURE;
        else if (!strcmp(tok, PRIMCOL))
            type = TYPE_PRIMCOL;
        else if (!strcmp(tok, OMIT))
            type = TYPE_OMIT;
        
        // If we don't know the material type, stop
        if (type == TYPE_UNKNOWN)
        {
            char *readname, *readtype;
            readname = (char*)calloc(1, strlen(name)+1);
            readtype = (char*)calloc(1, strlen(tok)+1);
            if (readname == NULL || readtype == NULL)
                terminate("Error: Unable to allocate memory for material error message. Oh dear...\n");
            strcpy(readname, name);
            strcpy(readtype, tok);
            memset(strbuf, 0, STRBUFF_SIZE);
            sprintf(strbuf, "Error: Unknown material type '%s' in material '%s'\n", readtype, readname);
            terminate(strbuf);
        }
        
        // Create the material from the type
        switch (type)
        {
            case TYPE_TEXTURE:
                w = (short)atoi(strtok(NULL, " "));
                h = (short)atoi(strtok(NULL, " "));
                mat = add_texture(name, w, h);
                if (!global_quiet) printf("    Added texture '%s'\n", name);
                break;
            case TYPE_PRIMCOL:
                r = (color)atoi(strtok(NULL, " "));
                g = (color)atoi(strtok(NULL, " "));
                b = (color)atoi(strtok(NULL, " "));
                mat = add_primcol(name, r, g, b);
                if (!global_quiet) printf("    Added primitive color '%s'\n", name);
                break;
            case TYPE_OMIT:
                mat = (n64Material*)calloc(1, sizeof(n64Material));
                if (mat == NULL)
                    terminate("Error: Unable to allocate memory for none type material\n");
                mat->name = (char*)calloc(1, strlen(name)+1);
                if (mat->name == NULL)
                    terminate("Error: Unable to allocate memory for none material name\n");
                strcpy(mat->name, name);
                mat->type = TYPE_OMIT;
                list_append(&list_materials, mat);
                break;
        }
        
        // Parse the rest of the arguments
        for (tok = strtok(NULL, " \n\r"); tok != NULL; tok = strtok(NULL, " \n\r"))
            mat_setflag(mat, tok);
    }
    
    // Close the file, as it's done being parsed
    if (!global_quiet) printf("Finished parsing materials file\n");
    fclose(fp);
    return mat;
}


/*==============================
    request_material
    Requests information regarding a new material from the user
    @param The name of the new material
    @returns A pointer to the new n64Material 
==============================*/

n64Material* request_material(char* name)
{
    char* tok;
    char strbuf[STRBUFF_SIZE];
    matType type;
    short w, h;
    color r, g, b;
    n64Material* mat;
    
    // Request the material type
    printf("New material '%s' found, please specify the following:\n", name);
    printf("\tMaterial type (0 = omit, 1 = texture, 2 = primitive color): ");
    scanf("%d", (int*)&type);
    
    // Create the material from the type
    switch (type)
    {
        case TYPE_TEXTURE:
            printf("\tTexture Width: ");
            scanf("%d", (int*)&w); clean_stdin();
            printf("\tTexture Height: ");
            scanf("%d", (int*)&h); clean_stdin();
            mat = add_texture(name, w, h);
            printf("\tMaterial flags (separate by spaces): ");
            fgets(strbuf, STRBUFF_SIZE, stdin);
            for (tok = strtok(strbuf, " \n\r"); tok != NULL; tok = strtok(NULL, " \n\r"))
                mat_setflag(mat, tok);
            if (!global_quiet) printf("Added texture '%s'\n", name);
            break;
        case TYPE_PRIMCOL:
            printf("\tPrimitve Red: ");
            scanf("%d", (int*)&r); clean_stdin();
            printf("\tPrimitve Green: ");
            scanf("%d", (int*)&g); clean_stdin();
            printf("\tPrimitve Blue: ");
            scanf("%d", (int*)&b); clean_stdin();
            mat = add_primcol(name, r, g, b);
            printf("\tMaterial flags (separate by spaces): ");
            fgets(strbuf, STRBUFF_SIZE, stdin);
            for (tok = strtok(strbuf, " \n\r"); tok != NULL; tok = strtok(NULL, " \n\r"))
                mat_setflag(mat, tok);
            if (!global_quiet) printf("Added primitive color '%s'\n", name);
            break;
        case TYPE_OMIT:
            mat = (n64Material*)calloc(1, sizeof(n64Material));
            if (mat == NULL)
                terminate("Error: Unable to allocate memory for none type material\n");
            mat->name = (char*)calloc(1, strlen(name)+1);
            if (mat->name == NULL)
                terminate("Error: Unable to allocate memory for none material name\n");
            strcpy(mat->name, name);
            mat->type = TYPE_OMIT;
            list_append(&list_materials, mat);
            if (!global_quiet) printf("Omitting material '%s'\n", name);
            return mat;
        default:
            sprintf(strbuf, "Error: Unknown material type '%d'\n", (int)type);
            terminate(strbuf);
    }
    return mat;
}
#pragma GCC diagnostic pop

/*==============================
    mat_setflag
    Allows you to modify a material flag
    @param The material to change
    @param The flag to set
==============================*/

void mat_setflag(n64Material* mat, char* flag)
{
    // Static variables to keep track of stuff between sucessive calls to this function
    char* copy;
    static n64Material* lastmat = NULL;
    static bool render2 = FALSE;
    static bool combine2 = FALSE;
    static bool texmode2 = FALSE;
    
    // Make a copy of the string
    copy = (char*)calloc(1, strlen(flag)+1);
    if (copy == NULL)
        terminate("Error: Unable to allocate memory for flag name copy\n");
    strcpy(copy, flag);
    
    // If our texture changed, reset the last flags
    if (lastmat != mat)
    {
        render2 = FALSE;
        combine2 = FALSE;
        texmode2 = FALSE;
        lastmat = mat;
    }
    
    // Set the material flags
    if (!strncmp(copy, DONTLOAD, sizeof(DONTLOAD)-1))
    {
        mat->dontload = TRUE;
    }
    else if (!strncmp(copy, LOADFIRST, sizeof(LOADFIRST)-1))
    {
        mat->loadfirst = TRUE;
    }
    else if (!strncmp(copy, G_CYC_, sizeof(G_CYC_)-1))
    {
        mat->cycle = copy;
    }
    else if (!strncmp(copy, G_TF_, sizeof(G_TF_)-1))
    {
        mat->texfilter = copy;
    }
    else if (!strncmp(copy, G_CC_, sizeof(G_CC_)-1))
    {
        if (!combine2)
        {
            mat->combinemode1 = copy;
            combine2 = TRUE;
        }
        else
            mat->combinemode2 = copy;
    }
    else if (!strncmp(copy, G_RM_, sizeof(G_RM_)-1))
    {
        if (!render2)
        {
            mat->rendermode1 = copy;
            render2 = TRUE;
        }
        else
            mat->rendermode2 = copy;
    }
    else if (!strncmp(copy, G_IM_FMT_, sizeof(G_IM_FMT_)-1))
    {
        if (mat->type != TYPE_TEXTURE)
            terminate("Error: Attempted to set image format on something that isn't a texture!\n");
        mat->data.image.coltype = copy;
    }
    else if (!strncmp(copy, G_IM_SIZ_, sizeof(G_IM_SIZ_)-1))
    {
        if (mat->type != TYPE_TEXTURE)
            terminate("Error: Attempted to set image bit size on something that isn't a texture!\n");
        mat->data.image.colsize = copy;
    }
    else if (!strncmp(copy, G_TX_, sizeof(G_TX_)-1))
    {
        if (mat->type != TYPE_TEXTURE)
            terminate("Error: Attempted to set texture mode on something that isn't a texture!\n");
        if (!texmode2)
        {
            mat->data.image.texmodes = copy;
            texmode2 = TRUE;
        }
        else
            mat->data.image.texmodet = copy;
    }
    else
    {
        int i;
        if (copy[0] != '!') // Set material flag
        {
            for (i=0; i<MAXGEOFLAGS; i++)
            {
                if (mat->geomode[i][0] == '\0')
                {
                    strcpy(mat->geomode[i], copy);
                    break;
                }
            }
        }
        else // Remove material flag
        {
            for (i=0; i<MAXGEOFLAGS; i++)
            {
                if (!strcmp(copy+1, mat->geomode[i]))
                {
                    memset(mat->geomode[i], 0, GEOFLAGSIZE);
                    break;
                }
            }        
        }
    }
}


/*==============================
    mat_hasgeoflag
    Checks if a material has a geometry flag
    @param The material to check
    @param The flag to check
==============================*/

bool mat_hasgeoflag(n64Material* mat, char* flag)
{
    int i;
    for (i=0; i<MAXGEOFLAGS; i++)
        if (!strcmp(flag, mat->geomode[i]))
            return TRUE;
    return FALSE;
}


/*==============================
    isvalidmat
    Checks if the material is valid
    IE it is not of type OMIT and does
    not have loading disabled
    @param The material to check
==============================*/

bool isvalidmat(n64Material* mat)
{
    return mat->type != TYPE_OMIT && !mat->dontload;
}


/*==============================
    get_validtexindex
    Since a list of materials contains multiple materials
    including stuff that has the OMIT flag or that is a 
    PRIMCOLOR, use this to find the index of valid textures
    only.
    @param   The list of materials to iterate
    @param   The name of the material to validate
    @returns The valid material index, or -1
==============================*/

int get_validtexindex(linkedList* materials, char* name)
{
    int index = 0;
    listNode* matnode;
    
    // Iterate through the mesh list
    for (matnode = materials->head; matnode != NULL; matnode = matnode->next)
    {
        n64Material* mat = (n64Material*)matnode->data;
        if (mat->type == TYPE_TEXTURE)
        {
            if (mat->dontload)
                continue;
            if (!strcmp(mat->name, name))
                return index;
            index++;
        }
    }
    return -1;
}


/*==============================
    get_validmatindex
    Since a list of materials contains multiple materials,
    including stuff that has the OMIT or DONTLOAD flag,
    use this to find the index of valid materials only.
    @param   The list of materials to iterate
    @param   The name of the material to validate
    @returns The valid material index, or -1
==============================*/

int get_validmatindex(linkedList* materials, char* name)
{
    int index = 0;
    listNode* matnode;
    
    // Iterate through the mesh list
    for (matnode = materials->head; matnode != NULL; matnode = matnode->next)
    {
        n64Material* mat = (n64Material*)matnode->data;
        if (mat->type != TYPE_OMIT)
        {
            if (mat->dontload)
                continue;
            if (!strcmp(mat->name, name))
                return index;
            index++;
        }
    }
    return -1;
}