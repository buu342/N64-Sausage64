#pragma once

typedef struct IUnknown IUnknown;

#include <string>
#include <list>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/filename.h>
#include "Include/glm/glm/glm.hpp"


/*********************************
              Macros
*********************************/

// For some reason this isn't defined in Windows?
#ifndef GL_MIRRORED_REPEAT
    #define GL_MIRRORED_REPEAT 0x8370
#endif
    
// Other useful stuff
#define G_CYC_     "G_CYC_"
#define G_TF_      "G_TF_"
#define G_CC_      "G_CC_"
#define G_RM_      "G_RM_"
#define G_IM_FMT_  "G_IM_FMT_"
#define G_IM_SIZ_  "G_IM_SIZ_"
#define G_TX_      "G_TX_"
#define DONTLOAD   "DONTLOAD"
#define LOADFIRST  "LOADFIRST"

// Default settings
#define DEFAULT_CYCLE          "G_CYC_1CYCLE"
#define DEFAULT_TEXFILTER      "G_TF_BILERP"
#define DEFAULT_COMBINE1_TEX   "G_CC_MODULATEIDECALA"
#define DEFAULT_COMBINE2_TEX   "G_CC_MODULATEIDECALA"
#define DEFAULT_COMBINE1_PRIM  "G_CC_PRIMLITE"
#define DEFAULT_COMBINE2_PRIM  "G_CC_PRIMLITE"
#define DEFAULT_RENDERMODE1    "G_RM_AA_ZB_OPA_SURF"
#define DEFAULT_RENDERMODE2    "G_RM_AA_ZB_OPA_SURF2"
#define DEFAULT_IMAGEFORMAT    "G_IM_FMT_RGBA"
#define DEFAULT_IMAGESIZE      "G_IM_SIZ_16b"
#define DEFAULT_TEXFLAGS       "G_TX_MIRROR"
#define DEFAULT_TEXFLAGT       "G_TX_MIRROR"
#define DEFAULT_GEOFLAGS       {"G_SHADE", "G_ZBUFFER", "G_CULL_BACK", "G_SHADING_SMOOTH", "G_LIGHTING"}


/*********************************
           Custom Types
*********************************/

// Texture types
typedef enum {
    TYPE_UNKNOWN = 0,
    TYPE_TEXTURE = 1,
    TYPE_PRIMCOL = 2
} texType;
    
    
/*********************************
            Structures
*********************************/

// Texture image data
typedef struct {
    uint32_t glid;
    wxImage  wximg;
    wxBitmap wxbmp;
    size_t w;
    size_t h;
    std::string coltype;
    std::string colsize;
    std::string texmodes;
    std::string texmodet;
} texImage;
    
// Texture primitive color data
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} texCol;


/*********************************
             Classes
*********************************/

class n64Texture
{
    private:
    
    protected:
    
    public:
        std::string            name;
        std::string            cycle;
        std::string            rendermode1;
        std::string            rendermode2;
        std::string            combinemode1;
        std::string            combinemode2;
        std::list<std::string> geomode;
        std::string            texfilter;
        texType                type;
        void*                  data;
        bool                   dontload;
        bool                   loadfirst;
        n64Texture(texType type);
        ~n64Texture();
        texImage* GetTextureData();
        texCol* GetPrimColorData();
        void CreateDefaultTexture(uint32_t w = 0, uint32_t h = 0);
        void CreateDefaultPrimCol();
        void CreateDefaultUnknown();
        void SetImageFromFile(std::string path);
        void SetImageFromData(const unsigned char* data, size_t size, uint32_t w, uint32_t h);
        bool HasGeoFlag(std::string flag);
        void RegenerateTexture();
        void SetFlag(std::string flag);
};