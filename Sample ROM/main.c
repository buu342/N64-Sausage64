/***************************************************************
                            main.c
                               
Program entrypoint.
***************************************************************/

#include <nusys.h>
#include "config.h"
#include "stages.h"
#include "debug.h"


/*********************************
        Function Prototypes
*********************************/

static void callback_prenmi();
static void callback_vsync(int tasksleft);

// Controller data
NUContData contdata[1];


/*==============================
    mainproc
    Initializes the game
==============================*/

void mainproc(void)
{
    // Start by selecting the proper television
    if (TV_TYPE == PAL)
    {
        osViSetMode(&osViModeTable[OS_VI_FPAL_LAN1]);
        osViSetYScale(0.833);
        osViSetSpecialFeatures(OS_VI_DITHER_FILTER_ON | OS_VI_GAMMA_OFF | OS_VI_GAMMA_DITHER_OFF | OS_VI_DIVOT_ON);
    }
    else if (TV_TYPE == MPAL)
    {
        osViSetMode(&osViModeTable[OS_VI_MPAL_LAN1]);
        osViSetSpecialFeatures(OS_VI_DITHER_FILTER_ON | OS_VI_GAMMA_OFF | OS_VI_GAMMA_DITHER_OFF | OS_VI_DIVOT_ON);
    }
    
    // Initialize and activate the graphics thread and Graphics Task Manager.
    nuGfxInit();
    
    // Initialize the controller
    nuContInit();
    
    // Initialize the debug library
    debug_initialize();
    debug_addcommand("ListAnims", "List the animations", command_listanims);
    debug_addcommand("ListFaces", "List the faces", command_listfaces);
    debug_addcommand("SetAnim", "Set the animation", command_setanim);
    debug_addcommand("SetFace", "Set the face", command_setface);
    debug_addcommand("ToggleLight", "Toggle lighting", command_togglelight);
    debug_addcommand("FreezeLight", "Freeze the light position", command_freezelight);
    debug_addcommand("ToggleLerp", "Toggle animation interpolation", command_togglelerp);
    debug_addcommand("ToggleLoop", "Toggle animation looping", command_toggleloop);
    debug_addcommand("ToggleAxis", "Toggle floor axis", command_toggleaxis);
    debug_printcommands();
        
    // Initialize stage 0
    stage00_init();
        
    // Set callback functions for reset and graphics
    nuGfxFuncSet((NUGfxFunc)callback_vsync);
    
    // Turn on the screen and loop forever to keep the idle thread busy
    nuGfxDisplayOn();
    while(1)
        ;
}


/*==============================
    callback_vsync
    Code that runs on on the graphics
    thread
    @param The number of tasks left to execute
==============================*/

static void callback_vsync(int tasksleft)
{
    // Update the stage, then draw it when the RDP is ready
    stage00_update();
    if (tasksleft < 1)
        stage00_draw();
}


/*==============================
    callback_prenmi
    Code that runs when the reset button
    is pressed. Required to prevent crashes
==============================*/

static void callback_prenmi()
{
    nuGfxDisplayOff();
    osViSetYScale(1);
}