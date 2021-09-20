/***************************************************************
                           stage00.c
                               
Handles the first level of the game.
***************************************************************/

#include <nusys.h>
#include "config.h"
#include "helper.h"
#include "axisMdl.h"
#include "catherineTex.h"
#include "catherineMdl.h"
#include "catherineAnim.h"
#include "debug.h"


/*********************************
              Macros
*********************************/

#define USB_BUFFER_SIZE 256


/*********************************
        Function Prototypes
*********************************/

void handle_mesh(int part);
void draw_catherine();
void draw_menu();


/*********************************
             Globals
*********************************/

// Matricies and vectors
static Mtx projection, viewing, modeling;
static u16 normal;

// Animations
static u8 useanims;
static u32 animtick;
static Animation* curanim;
static KeyFrame* currentframe;
static KeyFrame* nextframe;

// Facial animations
static u32 fanimtick;
static u8 findex;
static FaceAnim* curfanim;

// Time between animation keyframes
static OSTime frametime;

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

// USB
static char uselight = TRUE;
static char freezelight = FALSE;
static char useinterpolate = TRUE;
static char usb_buffer[USB_BUFFER_SIZE];


/*==============================
    stage00_init
    Initialize the stage
==============================*/

void stage00_init(void)
{
    // Initialize the animations
    useanims = TRUE;
    animtick = 0;
    curanim = &anim_Walk;
    currentframe = &curanim->keyframes[0];
    nextframe = &curanim->keyframes[1];
    
    // Initialize the face animations
    findex = 0;
    fanimtick = 60;
    curfanim = &catherine_faces[0];
    
    // Initialize the next frame time
    frametime = osGetTime() + OS_USEC_TO_CYCLES(22222);
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
    
    
    /* -------- Animation Keyframes -------- */
    
    // If the frame time has elapsed
    if (frametime < osGetTime())
    {
        // Increment the animation tick
        animtick++;
        fanimtick--;
        frametime = osGetTime() + OS_USEC_TO_CYCLES(22222);
        
        // If we hit the next keyframe in our animation
        if (animtick == nextframe->framenumber)
        {
            int currnumber;
            int nextnumber;
            
            // Go to the next keyframe
            currentframe = nextframe;
            currnumber = (currentframe - &curanim->keyframes[0]);
            nextnumber = (currnumber+1)%(curanim->framecount);
            nextframe = &curanim->keyframes[nextnumber];
            
            // If we hit the end of the animation, cycle back to the start
            if (nextnumber == 0)
            {
                currentframe = &curanim->keyframes[0];
                nextframe = &curanim->keyframes[1];
                animtick = 0;
            }
        }
        
        // Face animation blinking
        if (curfanim->hasblink)
        {
            switch (fanimtick)
            {
                case 3:
                case 1:
                    findex = 1;
                    break;
                case 2:
                    findex = 2;
                    break;
                case 0:
                    findex = 0;
                    fanimtick = 60 + guRandom()%80;
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
        int menuyscale[4] = {TOTALANIMS+1, TOTALFACES, 2, 1};
    
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
                    if (cury != 0)
                    {
                        useanims = TRUE;
                        animtick = 0;
                        curanim = catherine_anims[cury-1].anim;
                        currentframe = &curanim->keyframes[0];
                        nextframe = &curanim->keyframes[1];
                    }
                    else
                        useanims = FALSE;
                    break;
                case 1:
                    fanimtick = 60;
                    findex = 0;
                    curfanim = &catherine_faces[cury];
                    break;
                case 2:
                    if (cury == 0)
                        uselight = !uselight;
                    else
                        freezelight = !freezelight;
                    break;
                case 3:
                    useinterpolate = !useinterpolate;
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
    float fmat1[4][4], fmat2[4][4];
    
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
    
    // Draw catherine
    draw_catherine();
    
    // Syncronize the RCP and CPU and specify that our display list has ended
    gDPFullSync(glistp++);
    gSPEndDisplayList(glistp++);
    
    // Ensure the chache lines are valid
    osWritebackDCache(&projection, sizeof(projection));
    osWritebackDCache(&modeling, sizeof(modeling));
    
    // Ensure we haven't gone over the display list size and start the graphics task
    debug_assert((glistp-glist) < GLIST_LENGTH);
    nuGfxTaskStart(glist, (s32)(glistp - glist) * sizeof(Gfx), NU_GFX_UCODE_F3DEX, NU_SC_NOSWAPBUFFER);
    
    // Draw the menu
    nuDebConClear(NU_DEB_CON_WINDOW0);
    if (menuopen)
        draw_menu();
    nuDebConDisp(NU_SC_SWAPBUFFER);
}


/*==============================
    handle_mesh
    Handles the positioning and 
    drawing of a mesh
    @param The MESH_ macro to draw
==============================*/

void handle_mesh(int part)
{
    // If animations are enabled
    if (useanims)
    {
        FrameData* cframe = &currentframe->framedata[part];
        FrameData* nframe = &nextframe->framedata[part];
        float l = ((float)(animtick-(currentframe->framenumber)))/((float)((nextframe->framenumber)-(currentframe->framenumber)));
        
        // Create the root matricies
        guTranslate(&mparts[part].mtx_iroot, -roots[part].pos[0], -roots[part].pos[1], -roots[part].pos[2]);
        guTranslate(&mparts[part].mtx_root, roots[part].pos[0], roots[part].pos[1], roots[part].pos[2]);
        
        // Create the rotation and translation matricies
        if (useinterpolate)
        {
            guRotateRPY(&mparts[part].mtx_rotation, 
                lerp(cframe->rot[0], nframe->rot[0], l), 
                lerp(cframe->rot[1], nframe->rot[1], l),
                lerp(cframe->rot[2], nframe->rot[2], l));
            guTranslate(&mparts[part].mtx_translation, 
                lerp(cframe->pos[0], nframe->pos[0], l), 
                lerp(cframe->pos[1], nframe->pos[1], l), 
                lerp(cframe->pos[2], nframe->pos[2], l));
        }
        else
        {
            guRotateRPY(&mparts[part].mtx_rotation, cframe->rot[0], cframe->rot[1], cframe->rot[2]);
            guTranslate(&mparts[part].mtx_translation, cframe->pos[0], cframe->pos[1], cframe->pos[2]);    
        }
        
        // Apply the matricies
        gSPMatrix(glistp++, OS_K0_TO_PHYSICAL(&mparts[part].mtx_root), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
        gSPMatrix(glistp++, OS_K0_TO_PHYSICAL(&mparts[part].mtx_translation), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
        gSPMatrix(glistp++, OS_K0_TO_PHYSICAL(&mparts[part].mtx_rotation), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
        gSPMatrix(glistp++, OS_K0_TO_PHYSICAL(&mparts[part].mtx_iroot), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    }
    
    // Draw the body part
    gSPDisplayList(glistp++, mparts[part].dl);
    
    // If animations are enabled
    if (useanims)
    {
        // Pop all the previous transformations
        gSPPopMatrix(glistp++, G_MTX_MODELVIEW);
        gSPPopMatrix(glistp++, G_MTX_MODELVIEW);
        gSPPopMatrix(glistp++, G_MTX_MODELVIEW);
        gSPPopMatrix(glistp++, G_MTX_MODELVIEW);
    }
}


/*==============================
    draw_catherine
    Draws Catherine
==============================*/

void draw_catherine()
{
    // Initialize the model matrix
    guMtxIdent(&modeling);
    gSPMatrix(glistp++, &modeling, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);

    // Initialize the RCP to draw stuff nicely
    gDPSetCycleType(glistp++, G_CYC_1CYCLE);
    gDPSetDepthSource(glistp++, G_ZS_PIXEL);
    gSPClearGeometryMode(glistp++,0xFFFFFFFF);
    gSPSetGeometryMode(glistp++, G_SHADE | G_ZBUFFER | G_CULL_BACK | G_SHADING_SMOOTH | G_LIGHTING);
    gSPTexture(glistp++, 0x8000, 0x8000, 0, G_TX_RENDERTILE, G_ON);
    gDPSetRenderMode(glistp++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF);
    gDPSetCombineMode(glistp++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetTexturePersp(glistp++, G_TP_PERSP);
    gDPSetTextureFilter(glistp++, G_TF_BILERP);
    gDPSetTextureConvert(glistp++, G_TC_FILT);
    gDPSetTextureLOD(glistp++, G_TL_TILE);
    gDPSetTextureDetail(glistp++, G_TD_CLAMP);
    gDPSetTextureLUT(glistp++, G_TT_NONE);
    
    // Draw an axis on the floor for directional reference
    gSPDisplayList(glistp++, gfx_axis);
    
    // Draw her hair
    gDPSetCombineMode(glistp++, G_CC_PRIMLITE, G_CC_PRIMLITE);
    gDPSetPrimColor(glistp++, 0, 0, 175, 42, 44, 255);
    gDPPipeSync(glistp++);
    handle_mesh(MESH_Ponytail);
    handle_mesh(MESH_Bang);
    
    // Draw her arms
    gDPSetPrimColor(glistp++, 0, 0, 60, 71, 119, 255);
    gDPPipeSync(glistp++);
    handle_mesh(MESH_LeftArm);
    handle_mesh(MESH_RightArm);
    
    // Draw her body
    gDPSetPrimColor(glistp++, 0, 0, 119, 83, 50, 255);
    handle_mesh(MESH_Chest);
    handle_mesh(MESH_Pelvis);
    
    // Draw her boots
    handle_mesh(MESH_LeftFemur);
    handle_mesh(MESH_RightFemur);
    handle_mesh(MESH_LeftFoot);
    handle_mesh(MESH_RightFoot);
    
    // Draw her arms
    gDPSetPrimColor(glistp++, 0, 0, 250, 210, 184, 255);
    gDPSetCombineMode(glistp++, G_CC_PRIMLITE, G_CC_PRIMLITE);
    gDPPipeSync(glistp++);
    handle_mesh(MESH_LeftForearm);
    handle_mesh(MESH_RightForearm);
    handle_mesh(MESH_LeftHand);
    handle_mesh(MESH_RightHand);
    
    // Draw her pants
    gDPSetPrimColor(glistp++, 0, 0, 66, 66, 66, 255);
    gDPPipeSync(glistp++);
    handle_mesh(MESH_LeftLeg);
    handle_mesh(MESH_RightLeg);
    
    // Draw her equipment
    handle_mesh(MESH_Sword);
    handle_mesh(MESH_Pad);
    
    // Finally, draw the head with our custom faces
    gDPSetCombineMode(glistp++, G_CC_MODULATEIDECALA, G_CC_MODULATEIDECALA);
    gDPLoadTextureBlock(glistp++, curfanim->faces[findex], G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 64, 0, G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    gDPPipeSync(glistp++);
    handle_mesh(MESH_Head);
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
    nuDebConTextPos(NU_DEB_CON_WINDOW0, 4, 5);
    nuDebConCPuts(NU_DEB_CON_WINDOW0, "None");
    for (i=0; i<TOTALANIMS; i++)
    {
        nuDebConTextPos(NU_DEB_CON_WINDOW0, 4, 6+i);
        nuDebConCPuts(NU_DEB_CON_WINDOW0, catherine_anims[i].name);
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
    
    // Add the none animation to the string
    sprintf(usb_buffer, "None\n");
    
    // Go through all the animations names and append them to the string
    for (i=0; i<TOTALANIMS; i++)
        sprintf(usb_buffer, "%s%s\n", usb_buffer, catherine_anims[i].name);

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
    
    // If the none animation is requested, then disable animations
    if (!strcmp("None", usb_buffer))
    {
        useanims = FALSE;
        return "Animations disabled.";
    }
    
    // Compare the animation names
    for (i=0; i<TOTALANIMS; i++)
    {
        if (!strcmp(catherine_anims[i].name, usb_buffer))
        {
            useanims = TRUE;
            animtick = 0;
            curanim = catherine_anims[i].anim;
            currentframe = &curanim->keyframes[0];
            nextframe = &curanim->keyframes[1];
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
            fanimtick = 60;
            findex = 0;
            curfanim = &catherine_faces[i];
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
    useinterpolate = !useinterpolate;
    return "Interpolation Toggled";
}