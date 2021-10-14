#ifndef _SAUSN64_TEXTURE_H
#define _SAUSN64_TEXTURE_H


    /*********************************
               Flag Macros
    *********************************/
        
    // Texture type
    #define TEXTURE    "TEXTURE"
    #define PRIMCOL    "PRIMCOL"
    #define OMIT       "OMIT"

    // Geo modes
    #define G_SHADE                 "G_SHADE"
    #define G_ZBUFFER               "G_ZBUFFER"
    #define G_LIGHTING              "G_LIGHTING"
    #define G_SHADING_              "G_SHADING_"
    #define G_CULL_                 "G_CULL_"
    #define G_FOG                   "G_FOG"
    #define G_SHADING_SMOOTH        "G_SHADING_SMOOTH"
    #define G_SHADING_FLAT          "G_SHADING_FLAT"
    #define G_TEXTURE_GEN           "G_TEXTURE_GEN"
    #define G_TEXTURE_GEN_LINEAR    "G_TEXTURE_GEN_LINEAR"
    #define G_CLIPPING              "G_CLIPPING"

    // Other useful stuff
    #define G_CYC_       "G_CYC_"
    #define G_TF_        "G_TF_"
    #define G_CC_        "G_CC_"
    #define G_RM_        "G_RM_"
    #define G_IM_FMT_    "G_IM_FMT_"
    #define G_IM_SIZ_    "G_IM_SIZ_"
    #define G_TX_        "G_TX_"


    /*********************************
           Default Flag Macros
    *********************************/
    
    #define DEFAULT_CYCLE         "G_CYC_1CYCLE"
    #define DEFAULT_TEXFILTER     "G_TF_BILERP"
    #define DEFAULT_COMBINE1_TEX  "G_CC_MODULATEIDECALA"
    #define DEFAULT_COMBINE2_TEX  "G_CC_MODULATEIDECALA"
    #define DEFAULT_COMBINE1_PRIM "G_CC_PRIMLITE"
    #define DEFAULT_COMBINE2_PRIM "G_CC_PRIMLITE"
    #define DEFAULT_RENDERMODE1   "G_RM_AA_ZB_OPA_SURF"
    #define DEFAULT_RENDERMODE2   "G_RM_AA_ZB_OPA_SURF2"
    #define DEFAULT_IMAGEFORMAT   "G_IM_FMT_RGBA"
    #define DEFAULT_IMAGESIZE     "G_IM_SIZ_16b"
    #define DEFAULT_TEXFLAGS      "G_TX_MIRROR"
    #define DEFAULT_TEXFLAGT      "G_TX_MIRROR"
    #define DEFAULT_GEOFLAGS      {"G_SHADE", "G_ZBUFFER", "G_CULL_BACK", "G_SHADING_SMOOTH", "G_LIGHTING", "", "", "", "", ""}
    

    /*********************************
               Custom Types
    *********************************/

    // Texture types
    typedef enum {
        TYPE_UNKNOWN = -1,
        TYPE_OMIT    = 0,
        TYPE_TEXTURE = 1,
        TYPE_PRIMCOL = 2
    } texType;


    /*********************************
                 Structs
    *********************************/
    
    // Texture image data
    typedef struct {
        short w;
        short h;
        char* coltype;
        char* colsize;
        char* texmodes;
        char* texmodet;
    } texImage;
    
    // Texture primitive color data
    typedef struct {
        color r;
        color g;
        color b;
        color a;
    } texCol;
    
    // Texture type union
    typedef union {
        texImage image;
        texCol   color;
    } texData;

    // Texture struct
    typedef struct {
        char*   name;
        char*   cycle;
        char*   rendermode1;
        char*   rendermode2;
        char*   combinemode1;
        char*   combinemode2;
        char    geomode[10][32];
        char*   texfilter;
        texType type;
        texData data;
    } n64Texture;


    /*********************************
                 Globals
    *********************************/
    
    extern n64Texture texture_none;

    
    /*********************************
                Functions
    *********************************/
    
    extern n64Texture* add_texture(char* name, short w, short h);
    extern n64Texture* add_primcol(char* name, color r, color g, color b);
    extern n64Texture* find_texture(char* name);
    extern n64Texture* parse_textures(FILE* fp);
    extern n64Texture* request_texture(char* name);
    extern void        tex_setflag(n64Texture* tex, char* flag);
    
#endif