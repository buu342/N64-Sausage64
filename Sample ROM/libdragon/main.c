/***************************************************************
                            main.c
                               
Program entrypoint.
***************************************************************/

#include <libdragon.h>
#include <GL/gl.h>
#include <GL/gl_integration.h>
#include <GL/glu.h>
#include <time.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <stdbool.h>

// Load Sausage64 and the model for our character
#include "sausage64.h"
#include "catherineMdl.h"


/*********************************
        Function Prototypes
*********************************/

void setup_scene();
void setup_catherine();
u8   catherine_predraw(u16 part);
void scene_tick();
void scene_render();
void catherine_lookat();


/*********************************
         Global variables
*********************************/

// Camera variables
float campos[3] = {0, -300, 100};
float camang[3] = {0, 1, 0}; 

// Catherine model buffers
static s64ModelHelper* catherine;
static sprite_t* catherine_textures[TEXTURECOUNT_Catherine];

// Catherine face textures
// Because the face texture was declared as DONTLOAD in Arabiki (so we can animate them in code), we have
// to declare these materials manually.
GLuint FaceTex;
static s64Texture matdata_FaceTex = {&FaceTex, 32, 64, GL_LINEAR, GL_MIRRORED_REPEAT_ARB, GL_MIRRORED_REPEAT_ARB};
static s64Material mat_CatherineFace = {TYPE_TEXTURE, &matdata_FaceTex, 1, 0, 1, 1, 1};
GLuint FaceBlink1Tex;
static s64Texture matdata_FaceBlink1Tex = {&FaceBlink1Tex, 32, 64, GL_LINEAR, GL_MIRRORED_REPEAT_ARB, GL_MIRRORED_REPEAT_ARB};
static s64Material mat_CatherineBlink1Face = {TYPE_TEXTURE, &matdata_FaceBlink1Tex, 1, 0, 1, 1, 1};
GLuint FaceBlink2Tex;
static s64Texture matdata_FaceBlink2Tex = {&FaceBlink2Tex, 32, 64, GL_LINEAR, GL_MIRRORED_REPEAT_ARB, GL_MIRRORED_REPEAT_ARB};
static s64Material mat_CatherineBlink2Face = {TYPE_TEXTURE, &matdata_FaceBlink2Tex, 1, 0, 1, 1, 1};

// Model animation state
static int32_t curanim;

// Catherine face animation variables
static uint32_t facetick;
static s64Material* facemat;
static long long facetime;

// Lookat
static char lookat_canseecam = false;
static float lookat_amount = 0.0f;

// Default OpenGL lighting
static const GLfloat default_ambient[4] = {0.0, 0.0, 0.0, 1.0f};
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
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);

    // Allocate a Z-Buffer
    surface_t zbuffer = surface_alloc(FMT_RGBA16, 320, 240);

    // Initialize OpenGL
    gl_init();
    setup_scene();

    // Initialize the controller subsystem
    joypad_init();

    // Initialize Catherine
    setup_catherine();

    // game loop
    while (1)
    {
        // Perform a game tick
        joypad_poll();
        scene_tick();

        // Get the next framebuffer
        surface_t *fb = display_get();

        // Attach RDP to the framebuffer and zbuffer
        rdpq_attach(fb, &zbuffer);

        // Render the scene
        scene_render();

        // Detach RDP and show the framebuffer on screen
        rdpq_detach_show();
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
    glEnable(GL_MULTISAMPLE_ARB);
    glAlphaFunc(GL_GREATER, 0.5f);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, default_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, default_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, default_specular);

    // Projection matrix
    float aspect_ratio = (float)display_get_width()/(float)display_get_height();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, aspect_ratio, 100.0, 1000.0);

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
    s64ModelData* mdl;
    char *catherine_texture_paths[TEXTURECOUNT_Catherine];
    
    // In order to load our model properly, we must first create a list of textures it will use
    // These are defined in catherineMdl.h, and the indices *MUST* be respected.
    // Failure to do so will result in sections of the model loading the wrong texture
    catherine_texture_paths[TEXTURE_BackTex] = "rom:/BackTex.sprite";
    catherine_texture_paths[TEXTURE_BootTex] = "rom:/BootTex.sprite";
    catherine_texture_paths[TEXTURE_ChestTex] = "rom:/ChestTex.sprite";
    catherine_texture_paths[TEXTURE_KnifeSheatheTex] = "rom:/KnifeSheatheTex.sprite";
    catherine_texture_paths[TEXTURE_PantsTex] = "rom:/PantsTex.sprite";
    for (uint32_t i=0; i<TEXTURECOUNT_Catherine; i++)
        catherine_textures[i] = sprite_load(catherine_texture_paths[i]);

    // Load the binary model from ROM
    mdl = sausage64_load_binarymodel("rom:/catherineMdl.bin", catherine_textures);
    //sausage64_unload_binarymodel(mdl);
    //mdl = sausage64_load_binarymodel("rom:/catherineMdl.bin", catherine_textures);

    // Generate the face textures, because we're loading in the face textures dynamically
    sausage64_load_texture((s64Texture*)mat_CatherineFace.data, sprite_load("rom:/FaceTex.sprite"));
    sausage64_load_texture((s64Texture*)mat_CatherineBlink1Face.data, sprite_load("rom:/FaceBlink1Tex.sprite"));
    sausage64_load_texture((s64Texture*)mat_CatherineBlink2Face.data, sprite_load("rom:/FaceBlink2Tex.sprite"));
    
    // Initialize Catherine's model, then set her animation and predraw function
    catherine = sausage64_inithelper(mdl);
    curanim = ANIMATION_Catherine_Walk;
    sausage64_set_anim(catherine, curanim);
    sausage64_set_predrawfunc(catherine, catherine_predraw);

    // Initialize the face animation variables
    facemat = &mat_CatherineFace;
    facetick = 60;
    facetime = timer_ticks() + TIMER_TICKS(22222);
}


/*==============================
    scene_tick
    Called once, before rendering 
    anything
==============================*/

void scene_tick()
{
    joypad_inputs_t input;
    joypad_buttons_t input_pressed;
    long long curtick = timer_ticks();

    // Advance Catherine's animation    
    sausage64_advance_anim(catherine, 0.5f);
    
    
    /* -------- Controller -------- */

    input = joypad_get_inputs(JOYPAD_PORT_1);
    input_pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);

    // Reset the camera when START is pressed
    if (input_pressed.start)
    {
        campos[0] = 0;
        campos[1] = -300;
        campos[2] = 100;
        camang[0] = 0;
        camang[1] = 1;
        camang[2] = 0;
    }

    // Handle camera movement
    if (input.btn.z)
    {
        float speed = input.stick_y/10;
        campos[0] += camang[0]*speed;
        campos[1] += camang[1]*speed;
        campos[2] += camang[2]*speed;
    }
    else if (input.btn.r)
    {
        float w;
        if (input.stick_x > 5 || input.stick_x < -5)
            camang[0] += -((float)input.stick_x)/2000;
        if (input.stick_y > 5 || input.stick_y < -5)
            camang[2] += -((float)input.stick_y)/2000;
        w = 1/sqrtf(camang[0]*camang[0] + camang[1]*camang[1] + camang[2]*camang[2]);
        camang[0] *= w;
        camang[1] *= w;
        camang[2] *= w;
    }
    else
    {
        if (input.stick_x > 5 || input.stick_x < -5)
        {
            float w;    
            float temp[3];
            float cross[3] = {0, 0, 1};
            float speed = input.stick_x/10;
            temp[0] = camang[1]*cross[2] - camang[2]*cross[1];
            temp[1] = camang[2]*cross[0] - camang[0]*cross[2];
            temp[2] = camang[0]*cross[1] - camang[1]*cross[0];
            w = sqrtf(temp[0]*temp[0] + temp[1]*temp[1] + temp[2]*temp[2]);
            if (w != 0)
                w = 1/w;
            else
                w = 0;
            campos[0] -= temp[0]*speed*w;
            campos[1] -= temp[1]*speed*w;
            campos[2] -= temp[2]*speed*w;
        }
        if (input.stick_y > 5 || input.stick_y < -5)
        {
            float w;
            float temp[3];
            float cross[3] = {1, 0, 0};
            float speed = input.stick_y/10;
            temp[0] = camang[1]*cross[2] - camang[2]*cross[1];
            temp[1] = camang[2]*cross[0] - camang[0]*cross[2];
            temp[2] = camang[0]*cross[1] - camang[1]*cross[0];
            w = sqrtf(temp[0]*temp[0] + temp[1]*temp[1] + temp[2]*temp[2]);
            if (w != 0)
                w = 1/w;
            else
                w = 0;
            campos[0] += temp[0]*speed*w;
            campos[1] += temp[1]*speed*w;
            campos[2] += temp[2]*speed*w;
        }
    }

    
    
    /* -------- Animation -------- */

    // Animation changing
    if (input_pressed.d_up)
    {
        curanim--;
        if (curanim < 0)
            curanim = ANIMATIONCOUNT_Catherine - 1;
        sausage64_set_anim(catherine, curanim);
    }
    else if (input_pressed.d_down)
    {
        curanim = (curanim + 1)%ANIMATIONCOUNT_Catherine;
        sausage64_set_anim(catherine, curanim);
    }
    
    // Handle lookat lerp
    if (input.btn.l && lookat_canseecam) 
        lookat_amount += 0.1*(1.0 - lookat_amount);
    else
        lookat_amount -= 0.1*(lookat_amount);
    
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
    gl_context_begin();

    // Initialize the buffer with a color
    glClearColor(0.3f, 0.1f, 0.6f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

    // Initialize our view
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(campos[0], campos[1], campos[2], campos[0]+camang[0], campos[1]+camang[1], campos[2]+camang[2], 0, 0, 1);

    // Libdragon doesn't support retrieving the view/projection matrix state, so
    // for billboarding we need to provide the camera position relative to the model's root
    // Because the model is always at 0,0,0 in this example, we don't need to subtract the model root to get the relative pos
    // So we can just feed campos directly :)
    sausage64_set_camera(campos);
    
    // Make Catherine look at the camera
    catherine_lookat();

    // Render our model
    sausage64_drawmodel(catherine);

    gl_context_end();
}


/*********************************
     Model callback functions
*********************************/

/*==============================
    catherine_predraw
    Called before Catherine is drawn.
    This is needed since the model's face texture is not loaded
    automatically (due to the DONTLOAD flag). This instead allows
    us to swap the face dynamically (in this case, to let her blink).
    @param  The model segment being drawn
    @return 0 to disable the model drawing, 1 otherwise
==============================*/

u8 catherine_predraw(u16 part)
{
    // Handle face drawing
    switch (part)
    {
        case MESH_Catherine_Head:
            sausage64_loadmaterial(facemat);
            break;
    }
    return 1;
}


/*==============================
    catherine_lookat
    Make Catherine look at the camera
==============================*/

void catherine_lookat()
{
    float w;
    s64Transform* headtrans;
    float targetpos[3], targetdir[3];
    float eyepos[3] = {0, -25.27329f, 18.116f}; // This is the eye position, offset from the head mesh's root
    
    // First, we need the head's transform in model space
    headtrans = sausage64_get_meshtransform(catherine, MESH_Catherine_Head);
    
    // Now we can calculate the eye's position from the model's space
    // To make the code simpler, I am ignoring rotation and scaling of the head's transform
    // If you want to take the head's rotation into account, check the s64vec_rotate function in sausage64.c
    eyepos[0] = headtrans->pos[0] + eyepos[0];
    eyepos[1] = headtrans->pos[1] + eyepos[1];
    eyepos[2] = headtrans->pos[2] + eyepos[2];
    
    // Take the camera's position in world space and convert it to the model's space
    // The model is always at (0,0,0) in this ROM, so nothing magical here :P
    targetpos[0] = campos[0] - 0;
    targetpos[1] = campos[1] - 0;
    targetpos[2] = campos[2] - 0;
    
    // Calculate the direction vector and normalize it
    targetdir[0] = targetpos[0] - eyepos[0];
    targetdir[1] = targetpos[1] - eyepos[1];
    targetdir[2] = targetpos[2] - eyepos[2];
    w = 1/sqrtf(targetdir[0]*targetdir[0] + targetdir[1]*targetdir[1] + targetdir[2]*targetdir[2]);
    targetdir[0] *= w;
    targetdir[1] *= w;
    targetdir[2] *= w;
    
    // Put a limit on how much Catherine can turn her head
    // I'm just gonna check the angle between the target direction and the forward axis ((0, -1, 0) in Catherine's case)
    // instead of doing it properly by limiting pitch yaw and roll specifically.
    // Remember, if the head mesh rotates, the axis to check against must be properly rotated too. 
    w = 1/((targetdir[0]*targetdir[0] + targetdir[1]*targetdir[1] + targetdir[2]*targetdir[2]) * (0*0 + (-1)*(-1) + 0*0));
    w = (targetdir[0]*0 + targetdir[1]*(-1) + targetdir[2]*0)*w;
    lookat_canseecam = (w >= 0.6);
    
    // Perform the lookat
    sausage64_lookat(catherine, MESH_Catherine_Head, targetdir, lookat_amount, true);
}