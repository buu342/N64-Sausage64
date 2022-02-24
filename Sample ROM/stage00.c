/***************************************************************
                           stage00.c
                               
Handles the first level of the game.
***************************************************************/

#include <nusys.h>
#include <string.h> // Needed for CrashSDK compatibility
#include "config.h"
#include "helper.h"
#include "sausage64.h"
#include "axisMdl.h"
#include "catherineTex.h"
#include "catherineMdl.h"
#include "debug.h"


/*********************************
              Macros
*********************************/

#define USB_BUFFER_SIZE 256


/*********************************
        Function Prototypes
*********************************/

void draw_menu();
void catherine_predraw(u16 part);
void catherine_animcallback(u16 anim);

void matrix_inverse(float mat[4][4], float dest[4][4]);


/*********************************
             Globals
*********************************/

// Matricies and vectors
static Mtx projection, viewing, modeling;
static u16 normal;
// Lights
static Light light_amb;
static Light light_dir;

// Menu
static char menuopen = FALSE;
static s8   curx = 0;
static s8   cury = 0;

// Camera
static float campos[3] = {0, -100, -300};
static float camang[3] = {0, 0, -90};

// Catherine
Mtx catherineMtx[MESHCOUNT_Catherine];
s64ModelHelper catherine;
float catherine_animspeed;

// Face animation
static u16 faceindex;
static u32 facetick;
static OSTime facetime;
static FaceAnim* faceanim;

// USB
static char uselight = TRUE;
static char drawaxis = TRUE;
static char freezelight = FALSE;
static char usb_buffer[USB_BUFFER_SIZE];


/*==============================
    stage00_init
    Initialize the stage
==============================*/

void stage00_init(void)
{
    // Initialize Catherine
    sausage64_initmodel(&catherine, MODEL_Catherine, catherineMtx);
    sausage64_set_anim(&catherine, ANIMATION_Catherine_Walk); 
    sausage64_set_predrawfunc(&catherine, catherine_predraw);
    sausage64_set_animcallback(&catherine, catherine_animcallback);
    
    // Set catherine's animation speed based on region
    #if TV_TYPE == PAL
        catherine_animspeed = 0.66;
    #else
        catherine_animspeed = 0.5;
    #endif
    
    
    // Initialize the face animation
    facetick = 60;
    faceindex = 0;
    facetime = osGetTime() + OS_USEC_TO_CYCLES(22222);
    faceanim = &catherine_faces[0];
}


/*==============================
    stage00_update
    Update stage variables every frame
==============================*/

void stage00_update(void)
{
    int i;
    
    // Poll for USB commands
    debug_pollcommands();  
    
    // Advance Catherine's animation
    sausage64_advance_anim(&catherine, catherine_animspeed);
    
    
    /* -------- Face Animation -------- */
    
    // If the frame time has elapsed
    if (facetime < osGetTime())
    {
        // Advance the face animation tick
        facetick--;
        facetime = osGetTime() + OS_USEC_TO_CYCLES(22222);
        
        // Face animation blinking
        if (faceanim->hasblink)
        {
            switch (facetick)
            {
                case 3:
                case 1:
                    faceindex = 1;
                    break;
                case 2:
                    faceindex = 2;
                    break;
                case 0:
                    faceindex = 0;
                    facetick = 60 + guRandom()%80;
                    break;
            }
        }
    }
    
    
    /* -------- Controller -------- */
    
    // Read the controller
    nuContDataGetEx(contdata, 0);
    
    // Reset the camera when START is pressed
    if (contdata[0].trigger & START_BUTTON)
    {
        campos[0] = 0;
        campos[1] = -100;
        campos[2] = -300;
        camang[0] = 0;
        camang[1] = 0;
        camang[2] = -90;
    }
    
    // Toggle the menu when L is pressed
    if (contdata[0].trigger & L_TRIG)
        menuopen = !menuopen;
    
    // Handle camera movement and rotation
    if (contdata[0].button & Z_TRIG)
    {
        campos[2] += contdata->stick_y/10;
    }
    else if (contdata[0].button & R_TRIG)
    {
        camang[0] += contdata->stick_x/10;
        camang[2] -= contdata->stick_y/10;
    }
    else
    {
        campos[0] += contdata->stick_x/10;
        campos[1] += contdata->stick_y/10;
    }
    
        
    /* -------- Menu -------- */
    
    // If the menu is open
    if (menuopen)
    {
        int menuyscale[4] = {ANIMATIONCOUNT_Catherine, TOTALFACES, 2, 5};
    
        // Moving the cursor left/right
        if (contdata[0].trigger & R_JPAD)
            curx = (curx+1)%4;
        if (contdata[0].trigger & L_JPAD)
        {
            curx--;
            if (curx < 0)
                curx = 3;
        }
        
        // Moving the cursor up/down
        if (contdata[0].trigger & D_JPAD)
            cury++;
        if (contdata[0].trigger & U_JPAD)
            cury--;
            
        // Correct the Y position
        if (cury < 0)
            cury = menuyscale[curx]-1;
        cury %= menuyscale[curx];
        
        // Pressing A to do stuff
        if (contdata[0].trigger & A_BUTTON)
        {
            switch (curx)
            {
                case 0:
                    sausage64_set_anim(&catherine, cury);
                    break;
                case 1:
                    facetick = 60;
                    faceindex = 0;
                    faceanim = &catherine_faces[cury];
                    break;
                case 2:
                    if (cury == 0)
                        uselight = !uselight;
                    else
                        freezelight = !freezelight;
                    break;
                case 3:
                    if (cury == 0)
                        catherine.interpolate = !catherine.interpolate;
                    else if (cury == 1)
                        catherine.loop = !catherine.loop;
                    else if (cury == 2)
                        drawaxis = !drawaxis;
                    else if (cury == 3)
                        catherine_animspeed += 0.1;
                    else if (cury == 4)
                        catherine_animspeed -= 0.1;
                    break;
            }
        }
    }
}


/*==============================
    stage00_draw
    Draw the stage
==============================*/

void stage00_draw(void)
{
    int i, ambcol = 100;
    float fmat1[4][4], fmat2[4][4], w;
    
    // Assign our glist pointer to our glist array for ease of access
    glistp = glist;

    // Initialize the RCP and framebuffer
    rcp_init();
    fb_clear(0, 128, 0);
    
    // Setup the projection matrix
    guPerspective(&projection, &normal, 45, (float)SCREEN_WD / (float)SCREEN_HT, 10.0, 1000.0, 0.01);
    
    // Rotate and position the view
    guMtxIdentF(fmat1);
    guRotateRPYF(fmat2, camang[2], camang[0], camang[1]);
    guMtxCatF(fmat1, fmat2, fmat1);
    guTranslateF(fmat2, campos[0], campos[1], campos[2]);
    guMtxCatF(fmat1, fmat2, fmat1);
    guMtxF2L(fmat1, &viewing);
    
    // Apply the projection matrix
    gSPMatrix(glistp++, &projection, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(glistp++, &viewing, G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);
    gSPPerspNormalize(glistp++, &normal);
    
    // Setup the Sausage64 camera for billboarding
    sausage64_set_camera(&viewing, &projection);
    
    // Setup the lights
    if (!uselight)
        ambcol = 255;
    for (i=0; i<3; i++)
    {
        light_amb.l.col[i] = ambcol;
        light_amb.l.colc[i] = ambcol;
        light_dir.l.col[i] = 255;
        light_dir.l.colc[i] = 255;
    }
    
    // Calculate the light direction so it's always projecting from the camera's position
    if (!freezelight)
    {
        light_dir.l.dir[0] = -127*sinf(camang[0]*0.0174532925);
        light_dir.l.dir[1] = 127*sinf(camang[2]*0.0174532925)*cosf(camang[0]*0.0174532925);
        light_dir.l.dir[2] = 127*cosf(camang[2]*0.0174532925)*cosf(camang[0]*0.0174532925);
    }
    
    // Send the light struct to the RSP
    gSPNumLights(glistp++, NUMLIGHTS_1);
    gSPLight(glistp++, &light_dir, 1);
    gSPLight(glistp++, &light_amb, 2);
    gDPPipeSync(glistp++);
    
    // Initialize the model matrix
    guMtxIdent(&modeling);
    gSPMatrix(glistp++, &modeling, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);

    // Initialize the RCP to draw stuff nicely
    gDPSetCycleType(glistp++, G_CYC_1CYCLE);
    gDPSetDepthSource(glistp++, G_ZS_PIXEL);
    gSPClearGeometryMode(glistp++,0xFFFFFFFF);
    gSPSetGeometryMode(glistp++, G_SHADE | G_ZBUFFER | G_CULL_BACK | G_SHADING_SMOOTH | G_LIGHTING);
    gSPTexture(glistp++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetRenderMode(glistp++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF);
    gDPSetCombineMode(glistp++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetTexturePersp(glistp++, G_TP_PERSP);
    gDPSetTextureFilter(glistp++, G_TF_BILERP);
    gDPSetTextureConvert(glistp++, G_TC_FILT);
    gDPSetTextureLOD(glistp++, G_TL_TILE);
    gDPSetTextureDetail(glistp++, G_TD_CLAMP);
    gDPSetTextureLUT(glistp++, G_TT_NONE);
    
    // Draw an axis on the floor for directional reference
    if (drawaxis)
        gSPDisplayList(glistp++, gfx_axis);
    
    // Draw catherine
    sausage64_drawmodel(&glistp, &catherine);
    
    // Syncronize the RCP and CPU and specify that our display list has ended
    gDPFullSync(glistp++);
    gSPEndDisplayList(glistp++);
    
    // Ensure the chache lines are valid
    osWritebackDCache(&projection, sizeof(projection));
    osWritebackDCache(&modeling, sizeof(modeling));
    
    // Ensure we haven't gone over the display list size and start the graphics task
    debug_assert((glistp-glist) < GLIST_LENGTH);
    #if TV_TYPE != PAL
        nuGfxTaskStart(glist, (s32)(glistp - glist) * sizeof(Gfx), NU_GFX_UCODE_F3DEX, NU_SC_NOSWAPBUFFER);
    #else
        nuGfxTaskStart(glist, (s32)(glistp - glist) * sizeof(Gfx), NU_GFX_UCODE_F3DEX, NU_SC_SWAPBUFFER);
    #endif
    
    // Draw the menu (doesn't work on PAL)
    #if TV_TYPE != PAL
        nuDebConClear(NU_DEB_CON_WINDOW0);
        if (menuopen)
            draw_menu();
        nuDebConDisp(NU_SC_SWAPBUFFER);
    #endif
}


/*==============================
    draw_menu
    Draws the menu
==============================*/

void draw_menu()
{
    int i, cx;
    
    // List the animations
    nuDebConTextPos(NU_DEB_CON_WINDOW0, 3, 3);
    nuDebConCPuts(NU_DEB_CON_WINDOW0, "Anims");
    for (i=0; i<ANIMATIONCOUNT_Catherine; i++) // Can also use MODEL_Catherine->animcount (but macro is faster on the CPU)
    {
        nuDebConTextPos(NU_DEB_CON_WINDOW0, 4, 5+i);
        nuDebConCPuts(NU_DEB_CON_WINDOW0, MODEL_Catherine->anims[i].name);
    }
    
    // List the faces
    nuDebConTextPos(NU_DEB_CON_WINDOW0, 15, 3);
    nuDebConCPuts(NU_DEB_CON_WINDOW0, "Faces");
    for (i=0; i<TOTALFACES; i++)
    {
        nuDebConTextPos(NU_DEB_CON_WINDOW0, 16, 5+i);
        nuDebConCPuts(NU_DEB_CON_WINDOW0, catherine_faces[i].name);
    }
    
    // List the light options
    nuDebConTextPos(NU_DEB_CON_WINDOW0, 24, 3);
    nuDebConCPuts(NU_DEB_CON_WINDOW0, "Light");
    nuDebConTextPos(NU_DEB_CON_WINDOW0, 25, 5);
    nuDebConCPuts(NU_DEB_CON_WINDOW0, "Toggle");
    nuDebConTextPos(NU_DEB_CON_WINDOW0, 25, 6);
    nuDebConCPuts(NU_DEB_CON_WINDOW0, "Freeze");
    
    // List the other options
    nuDebConTextPos(NU_DEB_CON_WINDOW0, 32, 3);
    nuDebConCPuts(NU_DEB_CON_WINDOW0, "Other");
    nuDebConTextPos(NU_DEB_CON_WINDOW0, 33, 5);
    nuDebConCPuts(NU_DEB_CON_WINDOW0, "Lerp");
    nuDebConTextPos(NU_DEB_CON_WINDOW0, 33, 6);
    nuDebConCPuts(NU_DEB_CON_WINDOW0, "Loop");
    nuDebConTextPos(NU_DEB_CON_WINDOW0, 33, 7);
    nuDebConCPuts(NU_DEB_CON_WINDOW0, "Axis");
    nuDebConTextPos(NU_DEB_CON_WINDOW0, 33, 8);
    nuDebConCPuts(NU_DEB_CON_WINDOW0, "Faster");
    nuDebConTextPos(NU_DEB_CON_WINDOW0, 33, 9);
    nuDebConCPuts(NU_DEB_CON_WINDOW0, "Slower");
    
    // Draw a nice little bar separating everything
    nuDebConTextPos(NU_DEB_CON_WINDOW0, 3, 4);
    nuDebConCPuts(NU_DEB_CON_WINDOW0, "-------------------------------------");
    
    // Draw the cursor
    switch(curx)
    {
        case 0: cx = 3;  break;
        case 1: cx = 15; break;
        case 2: cx = 24; break;
        case 3: cx = 32; break;
    }
    nuDebConTextPos(NU_DEB_CON_WINDOW0, cx, 5+cury);
    nuDebConCPuts(NU_DEB_CON_WINDOW0, ">");
}


/*********************************
     Model callback functions
*********************************/

/*==============================
    catherine_predraw
    Called before Catherine is drawn
    @param The model segment being drawn
==============================*/

void catherine_predraw(u16 part)
{
    // Handle face drawing
    switch (part)
    {
        case MESH_Catherine_Head:
            gDPLoadTextureBlock(glistp++, faceanim->faces[faceindex], G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 64, 0, G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            break;
    }
}


/*==============================
    catherine_animcallback
    Called before an animation finishes
    @param The animation that is finishing
==============================*/

void catherine_animcallback(u16 anim)
{
    // Go to idle animation when we finished attacking
    switch(anim)
    {
        case ANIMATION_Catherine_Attack1:
        case ANIMATION_Catherine_ThrowKnife:
            sausage64_set_anim(&catherine, ANIMATION_Catherine_Idle);
            break;
    }
}


/*********************************
      USB Command Functions
*********************************/

/*==============================
    command_listanims
    USB Command for listing animations
==============================*/

char* command_listanims()
{
    int i;
    memset(usb_buffer, 0, USB_BUFFER_SIZE);

    // Go through all the animations names and append them to the string
    for (i=0; i<ANIMATIONCOUNT_Catherine; i++)
        sprintf(usb_buffer, "%s%s\n", usb_buffer, MODEL_Catherine->anims[i].name);

        // Return the string of animation names
    return usb_buffer;
}


/*==============================
    command_setanim
    USB Command for setting animations
==============================*/

char* command_setanim()
{
    int i;
    memset(usb_buffer, 0, USB_BUFFER_SIZE);
    
    // Check the animation name isn't too big
    if (debug_sizecommand() > USB_BUFFER_SIZE)
        return "Name larger than USB buffer";
    debug_parsecommand(usb_buffer);
    
    // Compare the animation names
    for (i=0; i<ANIMATIONCOUNT_Catherine; i++)
    {
        if (!strcmp(MODEL_Catherine->anims[i].name, usb_buffer))
        {
            sausage64_set_anim(&catherine, i);
            return "Animation set.";
        }
    }

    // No animation found
    return "Unkown animation name";
}


/*==============================
    command_listfaces
    USB Command for listing faces
==============================*/

char* command_listfaces()
{
    int i;
    memset(usb_buffer, 0, USB_BUFFER_SIZE);
    
    for (i=0; i<TOTALFACES; i++)
        sprintf(usb_buffer, "%s%s\n", usb_buffer, catherine_faces[i].name);

    return usb_buffer;
}


/*==============================
    command_setface
    USB Command for setting faces
==============================*/

char* command_setface()
{
    int i;
    memset(usb_buffer, 0, USB_BUFFER_SIZE);
    
    // Check the face name isn't too big
    if (debug_sizecommand() > USB_BUFFER_SIZE)
        return "Name larger than USB buffer";
    debug_parsecommand(usb_buffer);
    
    // Compare the face names
    for (i=0; i<TOTALFACES; i++)
    {
        if (!strcmp(catherine_faces[i].name, usb_buffer))
        {
            facetick = 60;
            faceindex = 0;
            faceanim = &catherine_faces[i];
            return "Face set!";
        }
    }
    return "Unknown face name";
}


/*==============================
    command_togglelight
    USB Command for toggling lighting
==============================*/

char* command_togglelight()
{
    uselight = !uselight;
    return "Light Toggled";
}


/*==============================
    command_freezelight
    USB Command for freezing lighting
==============================*/

char* command_freezelight()
{
    freezelight = !freezelight;
    return "Light state altered";
}


/*==============================
    command_togglelerp
    USB Command for toggling lerp
==============================*/

char* command_togglelerp()
{
    catherine.interpolate = !catherine.interpolate;
    return "Interpolation Toggled";
}


/*==============================
    command_toggleloop
    USB Command for toggling animation looping
==============================*/

char* command_toggleloop()
{
    catherine.loop = !catherine.loop;
    return "Loop Toggled";
}


/*==============================
    command_toggleaxis
    USB Command for toggling the floor axis
==============================*/

char* command_toggleaxis()
{
    drawaxis = !drawaxis;
    return "Axis Toggled";
}