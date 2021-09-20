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


/*********************************
             Globals
*********************************/

// Matricies and vectors
static Mtx projection, viewing, modeling;
static u16 normal;

// Animations
static u32 animtick;
static Animation* curanim;
static KeyFrame* currentframe;
static KeyFrame* nextframe;

// Facial animations
static u32 fanimtick;
static u8 findex;
static FaceAnim* curfanim;

// Time between animation keyframes
static OSTime frametime = 0;

// Lights
static Light light_amb;
static Light light_dir;

// USB
static char uselight = TRUE;
static char freezelight = FALSE;
static char usb_buffer[USB_BUFFER_SIZE];

// Camera
static float campos[3] = {0, -100, -300};
static float camang[3] = {0, 0, -90};


/*==============================
    stage00_init
    Initialize the stage
==============================*/

void stage00_init(void)
{
    findex = 0;
    animtick = 0;
    fanimtick = 60;
    curanim = &anim_Walk;
    curfanim = &catherine_faces[0];
    currentframe = &curanim->keyframes[0];
    nextframe = &curanim->keyframes[1];
    frametime = osGetTime() + OS_USEC_TO_CYCLES(22222);
}


/*==============================
    stage00_update
    Update stage variables every frame
==============================*/

void stage00_update(void)
{
    int i;
    
    debug_pollcommands();
    
    /* -------- Animation Keyframes -------- */
    
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
        for (i=0; i<3; i++)
        {
            campos[i] = 0;
            camang[i] = 0;
        }
    }
    
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
    
    // Rotate the view
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
    nuGfxTaskStart(glist, (s32)(glistp - glist) * sizeof(Gfx), NU_GFX_UCODE_F3DEX, NU_SC_SWAPBUFFER);
}


/*==============================
    handle_mesh
    Handles the positioning and 
    drawing of a mesh
    @param The MESJ_ macro to draw
==============================*/

void handle_mesh(int part)
{
    FrameData* cframe = &currentframe->framedata[part];
    FrameData* nframe = &nextframe->framedata[part];
    float l = ((float)(animtick-(currentframe->framenumber)))/((float)((nextframe->framenumber)-(currentframe->framenumber)));
    
    // Create the root matricies
    guTranslate(&mparts[part].mtx_iroot, -roots[part].pos[0], -roots[part].pos[1], -roots[part].pos[2]);
    guTranslate(&mparts[part].mtx_root, roots[part].pos[0], roots[part].pos[1], roots[part].pos[2]);
    
    // Create the rotation and translation matricies
    guRotateRPY(&mparts[part].mtx_rotation, 
        lerp(cframe->rot[0], nframe->rot[0], l), 
        lerp(cframe->rot[1], nframe->rot[1], l),
        lerp(cframe->rot[2], nframe->rot[2], l));
    guTranslate(&mparts[part].mtx_translation, 
        lerp(cframe->pos[0], nframe->pos[0], l), 
        lerp(cframe->pos[1], nframe->pos[1], l), 
        lerp(cframe->pos[2], nframe->pos[2], l));
    
    // Apply the matricies
    gSPMatrix(glistp++, OS_K0_TO_PHYSICAL(&mparts[part].mtx_root), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPMatrix(glistp++, OS_K0_TO_PHYSICAL(&mparts[part].mtx_translation), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPMatrix(glistp++, OS_K0_TO_PHYSICAL(&mparts[part].mtx_rotation), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPMatrix(glistp++, OS_K0_TO_PHYSICAL(&mparts[part].mtx_iroot), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    
    // Draw the body part
    gSPDisplayList(glistp++, mparts[part].dl);
    
    // Pop all the previous transformations
    gSPPopMatrix(glistp++, G_MTX_MODELVIEW);
    gSPPopMatrix(glistp++, G_MTX_MODELVIEW);
    gSPPopMatrix(glistp++, G_MTX_MODELVIEW);
    gSPPopMatrix(glistp++, G_MTX_MODELVIEW);
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
    command_listanims
    USB Command for listing animations
==============================*/

char* command_listanims()
{
    int i;
    memset(usb_buffer, 0, USB_BUFFER_SIZE);
    
    for (i=0; i<TOTALANIMS; i++)
        sprintf(usb_buffer, "%s%s\n", usb_buffer, catherine_anims[i].name);

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
    for (i=0; i<TOTALANIMS; i++)
    {
        if (!strcmp(catherine_anims[i].name, usb_buffer))
        {
            animtick = 0;
            curanim = catherine_anims[i].anim;
            currentframe = &curanim->keyframes[0];
            nextframe = &curanim->keyframes[1];
            return "Animation set!";
        }
    }
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