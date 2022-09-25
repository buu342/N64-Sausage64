/***************************************************************
                            main.c
                               
Program entrypoint.
***************************************************************/

#include <libdragon.h>
#include <GL/gl.h>
#include <GL/gl_integration.h>
#include <time.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>

// Load Sausage64, the model, and the textures for our character
#include "sausage64.h"
#include "catherineTex.h"
#include "catherineMdl.h"


/*********************************
        Function Prototypes
*********************************/

void setup_scene();
void setup_catherine();
void catherine_predraw(u16 part);
void generate_texture(s64Texture* tex, GLuint* store, sprite_t* texture);
void scene_tick();
void scene_render();


/*********************************
         Global variables
*********************************/

// Catherine model buffers
static s64ModelHelper catherine;
static GLuint catherine_buffers[MESHCOUNT_Catherine*2];
static sprite_t* catherine_textures[CATHERINE_TETXURE_COUNT];

// Catherine face textures
// Because the face texture was declared as DONTLOAD in Arabiki, we have
// to declare these materials manually.
static s64Texture matdata_FaceTex = {&FaceTex, 32, 64, GL_LINEAR, GL_MIRRORED_REPEAT_ARB, GL_MIRRORED_REPEAT_ARB};
static s64Material mat_CatherineFace = {TYPE_TEXTURE, &matdata_FaceTex, 1, 0, 1, 1, 1};
static s64Texture matdata_FaceBlink1Tex = {&FaceBlink1Tex, 32, 64, GL_LINEAR, GL_MIRRORED_REPEAT_ARB, GL_MIRRORED_REPEAT_ARB};
static s64Material mat_CatherineBlink1Face = {TYPE_TEXTURE, &matdata_FaceBlink1Tex, 1, 0, 1, 1, 1};
static s64Texture matdata_FaceBlink2Tex = {&FaceBlink2Tex, 32, 64, GL_LINEAR, GL_MIRRORED_REPEAT_ARB, GL_MIRRORED_REPEAT_ARB};
static s64Material mat_CatherineBlink2Face = {TYPE_TEXTURE, &matdata_FaceBlink2Tex, 1, 0, 1, 1, 1};

// Catherine face animation variables
static uint32_t facetick;
static s64Material* facemat;
static long long facetime;

// Default OpenGL lighting
static const GLfloat default_ambient[4] = {0.0f, 0.0f, 0.0f, 1.0f};
static const GLfloat default_diffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};
static const GLfloat default_specular[4] = {0.0f, 0.0f, 0.0f, 0.0f};


/*==============================
    main
    Initializes the game and
    performs the main loop
==============================*/

int main()
{
    // Initialize the debug libraries
    debug_init_isviewer();
    debug_init_usblog();
    
    // Initialize the timer subsystem
    timer_init();
    srand(time(NULL));

    // Initialize the screen    
    dfs_init(DFS_DEFAULT_LOCATION);
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);

    // Initialize OpenGL
    gl_init();
    setup_scene();

    // Initialize Catherine
    setup_catherine();

    // game loop
    while (1)
    {
        // Perform a game tick
        scene_tick();

        // Render the scene
        scene_render();
        gl_swap_buffers();
    }
}


/*==============================
    setup_scene
    Initializes OpenGL
==============================*/

void setup_scene()
{
    // Initial setup
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, default_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, default_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, default_specular);

    // Projection matrix
    float aspect_ratio = (float)display_get_width()/(float)display_get_height();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1*aspect_ratio, 1*aspect_ratio, -1, 1, 1, 300);

    // Modelview matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Light (directional)
    GLfloat light_pos[] = { 0, 0, 1, 0 };
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    GLfloat light_diffuse[] = { 1, 1, 1, 1 };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
}


/*==============================
    setup_catherine
    Initializes the catherine 
    model and buffers
==============================*/

void setup_catherine()
{
    // IMPORTANT! WE MUST SET THE GL BUFFERS TO 0xFF BEFORE USING THEM!!!
    // Sausage64 will not initialize correctly them unless they're 0xFFFFFFFF
    // Since you can reuse these vertex+face buffers between multiple s64ModelHelpers, you only need to do this once.
    memset(catherine_buffers, 0xFF, MESHCOUNT_Catherine*2*sizeof(GLuint));

    // Load Catherine's textures into sprite structures
    for (uint32_t i=0; i<CATHERINE_TETXURE_COUNT; i++)
        catherine_textures[i] = sprite_load(catherine_texture_paths[i]);
    
    // Initialize Catherine's model, then set her animation and predraw function
    sausage64_initmodel(&catherine, MODEL_Catherine, catherine_buffers);
    sausage64_set_anim(&catherine, ANIMATION_Catherine_Walk);
    sausage64_set_predrawfunc(&catherine, catherine_predraw);

    // Generate the textures
    generate_texture((s64Texture*)mat_BackTex.data, &BackTex, catherine_textures[0]);
    generate_texture((s64Texture*)mat_BootTex.data, &BootTex, catherine_textures[1]);
    generate_texture((s64Texture*)mat_ChestTex.data, &ChestTex, catherine_textures[2]);
    generate_texture((s64Texture*)mat_KnifeSheatheTex.data, &KnifeSheatheTex, catherine_textures[3]);
    generate_texture((s64Texture*)mat_PantsTex.data, &PantsTex, catherine_textures[4]);
    generate_texture((s64Texture*)mat_CatherineFace.data, &FaceTex, catherine_textures[5]);
    generate_texture((s64Texture*)mat_CatherineBlink1Face.data, &FaceBlink1Tex, catherine_textures[6]);
    generate_texture((s64Texture*)mat_CatherineBlink2Face.data, &FaceBlink2Tex, catherine_textures[7]);

    // Initialize the face animation variables
    facemat = &mat_CatherineFace;
    facetick = 60;
    facetime = timer_ticks() + TIMER_TICKS(22222);
}


/*==============================
    catherine_predraw
    Called before Catherine is drawn.
    This is needed since the model's face texture is not loaded
    automatically (due to the DONTLOAD flag). This instead allows
    us to swap the face dynamically (in this case, to let her blink).
    @param The model segment being drawn
==============================*/

void catherine_predraw(u16 part)
{
    // Handle face drawing
    switch (part)
    {
        case MESH_Catherine_Head:
            sausage64_loadmaterial(facemat);
            break;
    }
}


/*==============================
    generate_texture
    Generates a texture for OpenGL.
    Since the s64Texture struct contains a bunch of information,
    this function lets us create these textures with the correct
    attributes automatically.
    @param The Sausage64 texture
    @param The GLuint to store the texture in
    @param The texture data itself, in a sprite struct
==============================*/

void generate_texture(s64Texture* tex, GLuint* store, sprite_t* texture)
{
    // Create the texture buffer 
    glGenTextures(1, store);
    glBindTexture(GL_TEXTURE_2D, *store);

    // Set the texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tex->wraps);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tex->wrapt);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tex->filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tex->filter);

    // Generate mipmaps
    for (uint32_t i=0; i<4; i++)
    {
        surface_t surf = sprite_get_lod_pixels(texture, i);
        if (!surf.buffer) break;
        data_cache_hit_writeback(surf.buffer, surf.stride*surf.height);
        glTexImageN64(GL_TEXTURE_2D, i, &surf);
    }
}


/*==============================
    scene_tick
    Called once, before rendering 
    anything
==============================*/

void scene_tick()
{
    long long curtick = timer_ticks();

    // Advance Catherine's animation    
    sausage64_advance_anim(&catherine, 1.0f);
    
    // If the frame time has elapsed
    if (facetime < curtick)
    {
        // Advance the face animation tick
        facetick--;
        facetime = curtick + TIMER_TICKS(22222);
        switch (facetick)
        {
            case 3:
            case 1:
                facemat = &mat_CatherineBlink1Face;
                break;
            case 2:
                facemat = &mat_CatherineBlink2Face;
                break;
            case 0:
                facemat = &mat_CatherineFace;
                facetick = 60 + rand()%80;
                break;
            default:
                break;
        }
    }
}


/*==============================
    scene_render
    Renders the scene (and our model)
==============================*/

void scene_render()
{
    // Initialize the buffer with a color
    glClearColor(0.3f, 0.1f, 0.6f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

    // Initialize our view
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, -100, -150);

    // Apply a rotation to the scene
    glRotatef(45, 0, 1, 0);
    glRotatef(-90, 1, 0, 0);
    glPushMatrix();

    // Render our model
    sausage64_drawmodel(&catherine);
}