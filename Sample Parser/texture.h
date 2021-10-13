#ifndef _SAUSN64_TEXTURE_H
#define _SAUSN64_TEXTURE_H

    /*********************************
               Custom Types
    *********************************/

    // Texture types
    typedef enum {
        TYPE_TEXTURE = 0,
        TYPE_PRIMCOL = 1
    } texType;


    /*********************************
                 Structs
    *********************************/
    
    // Texture image data
    typedef struct {
        short w;
        short h;
        // Color Type
        // Color Size
        // Mode1
        // Mode2
    } texImage;
    
    // Texture primitive color data
    typedef struct {
        color r;
        color g;
        color b;
        char alignment;
    } texCol;    
    
    // Texture type union
    typedef union {
        texImage image;
        texCol   color;
    } texData;

    // Texture struct
    typedef struct {
        char* name;
        //char* rendermode;
        //char* combinemode;
        //bool nocull;
        //bool omit;
        texType type; // Use a string for the type when parsing
        texData data; // Probably change this to (void*)
    } n64Texture;
    
    
    /*********************************
                Functions
    *********************************/
    
    extern n64Texture* add_texture(char* name, short w, short h);
    extern n64Texture* add_primcol(char* name, color r, color g, color b);
    extern n64Texture* find_texture(char* name);
    extern n64Texture* parse_textures(FILE* fp);
    extern n64Texture* request_texture(char* name);
    
#endif