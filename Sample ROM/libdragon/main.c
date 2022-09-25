#include <libdragon.h>
#include <GL/gl.h>
#include <GL/gl_integration.h>
#include <malloc.h>
#include <math.h>

// Load Sausage64 and the textures for our character
#include "sausage64.h"
#include "catherineTex.h"
#include "catherineMdl.h"


/*================================================================ */

void setup();
void setup_catherine();
void catherine_predraw(u16 part);
void generate_texture(s64Texture* tex, GLuint* store, sprite_t* texture);
void render();


/*================================================================ */

static s64ModelHelper catherine;
static GLuint catherine_buffers[MESHCOUNT_Catherine*2];
static sprite_t* catherine_textures[CATHERINE_TETXURE_COUNT];

static s64Texture matdata_FaceTex = { &FaceTex, 32, 64, GL_LINEAR, GL_REPEAT, GL_REPEAT };
static s64Material mat_CatherineFace = { TYPE_TEXTURE, &matdata_FaceTex, 1, 0, 1, 1, 1 };

// Default OpenGL lighting
static const GLfloat default_ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
static const GLfloat default_diffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat default_specular[4] = { 0.0f, 0.0f, 0.0f, 0.0f };


/*================================================================ */

int main()
{
    // Initialize the debug libraries
    debug_init_isviewer();
    debug_init_usblog();
    
    // Initialize the screen    
    dfs_init(DFS_DEFAULT_LOCATION);
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);

    // Initialize OpenGL
    gl_init();
    setup();

    // Initialize Catherine
    setup_catherine();

    // Initialize the controller
    controller_init();

    // Program loop
    while (1)
    {
        // Get the controller state
        controller_scan();

        // Advance the sausage64 animation
        sausage64_advance_anim(&catherine, 1.0f);

        // Render the scene
        render();
        gl_swap_buffers();
    }
}

void setup()
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

void setup_catherine()
{
    // IMPORTANT! WE MUST SET THE GL BUFFERS TO 0xFF BEFORE USING THEM!!!
    // Sausage64 will not initialize them unless they're 0xFFFFFFFF
    memset(catherine_buffers, 0xFF, ANIMATIONCOUNT_Catherine*2*sizeof(GLuint));

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
}

void catherine_predraw(u16 part)
{
    // Handle face drawing
    switch (part)
    {
        case MESH_Catherine_Head:
            sausage64_loadmaterial(&mat_CatherineFace);
            break;
    }
}

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

void render()
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