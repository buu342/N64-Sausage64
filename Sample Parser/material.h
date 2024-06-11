#ifndef _SAUSN64_MATERIAL_H
#define _SAUSN64_MATERIAL_H


    /*********************************
               Flag Macros
    *********************************/
    
    // Max geometry flags
    #define MAXGEOFLAGS 10
    #define GEOFLAGSIZE 32
        
    // Texture type
    #define TEXTURE    "TEXTURE"
    #define PRIMCOL    "PRIMCOL"
    #define OMIT       "OMIT"
    
    // Other useful stuff
    #define G_CYC_       "G_CYC_"
    #define G_TF_        "G_TF_"
    #define G_CC_        "G_CC_"
    #define G_RM_        "G_RM_"
    #define G_IM_FMT_    "G_IM_FMT_"
    #define G_IM_SIZ_    "G_IM_SIZ_"
    #define G_TX_        "G_TX_"
    #define DONTLOAD     "DONTLOAD"
    #define LOADFIRST    "LOADFIRST"


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

    // Material types
    typedef enum {
        TYPE_UNKNOWN = -1,
        TYPE_OMIT    = 0,
        TYPE_TEXTURE = 1,
        TYPE_PRIMCOL = 2
    } matType;


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
    } matImage;
    
    // Texture primitive color data
    typedef struct {
        color r;
        color g;
        color b;
        color a;
    } matCol;
    
    // Texture type union
    typedef union {
        matImage image;
        matCol   color;
    } matData;

    // Texture struct
    typedef struct {
        char*   name;
        char*   cycle;
        char*   rendermode1;
        char*   rendermode2;
        char*   combinemode1;
        char*   combinemode2;
        char    geomode[MAXGEOFLAGS][GEOFLAGSIZE];
        char*   texfilter;
        bool    dontload;
        bool    loadfirst;
        matType type;
        matData data;
    } n64Material;


    /*********************************
                 Globals
    *********************************/
    
    extern n64Material material_none;

    
    /*********************************
                Functions
    *********************************/
    
    extern n64Material* add_texture(char* name, short w, short h);
    extern n64Material* add_primcol(char* name, color r, color g, color b);
    extern n64Material* find_material(char* name);
    extern n64Material* parse_materials(FILE* fp);
    extern n64Material* request_material(char* name);
    extern void        mat_setflag(n64Material* mat, char* flag);
    extern bool        mat_hasgeoflag(n64Material* mat, char* flag);
    extern bool        isvalidmat(n64Material* mat);
    extern int         get_validtexindex(linkedList* materials, char* name);
    extern int         get_validmatindex(linkedList* materials, char* name);
    
#endif